/*
 * "$Id$"
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
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "../../lib/libprintut.h"

#include "print_gimp.h"

#include <sys/types.h>
#include <signal.h>
#include <ctype.h>
#include <sys/wait.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "print-intl.h"

/*
 * Local functions...
 */

static void	printrc_load (void);
void		printrc_save (void);
static int	compare_printers (gp_plist_t *p1, gp_plist_t *p2);
static void	get_system_printers (void);

static void	query (void);
static void	run (char *, int, GimpParam *, int *, GimpParam **);
static int	do_print_dialog (char *proc_name);

/*
 * Work around GIMP library not being const-safe.  This is a very ugly
 * hack, but the excessive warnings generated can mask more serious
 * problems.
 */

#define BAD_CONST_CHAR char *

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

static gp_plist_t gimp_vars;

int		plist_current = 0,	/* Current system printer */
		plist_count = 0;	/* Number of system printers */
gp_plist_t	*plist;			/* System printers */

int		saveme = FALSE;		/* True if print should proceed */
int		runme = FALSE;		/* True if print should proceed */
stp_printer_t current_printer = 0;	/* Current printer index */
static gint32          image_ID;	        /* image ID */

const char *image_filename;

#define SAFE_FREE(x)				\
do						\
{						\
  if ((x))					\
    free((char *)(x));				\
  ((x)) = NULL;					\
} while (0)

void
plist_set_output_to(gp_plist_t *p, const char *val)
{
  if (p->output_to == val)
    return;
  SAFE_FREE(p->output_to);
  p->output_to = g_strdup(val);
}

void
plist_set_output_to_n(gp_plist_t *p, const char *val, int n)
{
  if (p->output_to == val)
    return;
  SAFE_FREE(p->output_to);
  p->output_to = g_strndup(val, n);
}

const char *
plist_get_output_to(const gp_plist_t *p)
{
  return p->output_to;
}

void
plist_set_name(gp_plist_t *p, const char *val)
{
  if (p->name == val)
    return;
  SAFE_FREE(p->name);
  p->name = g_strdup(val);
}

void
plist_set_name_n(gp_plist_t *p, const char *val, int n)
{
  if (p->name == val)
    return;
  SAFE_FREE(p->name);
  p->name = g_strndup(val, n);
}

const char *
plist_get_name(const gp_plist_t *p)
{
  return p->name;
}

void
initialize_printer(gp_plist_t *printer)
{
  plist_set_output_to(printer, "");
  plist_set_name(printer, "");
  printer->active = 0;
  printer->scaling = 100.0;
  printer->orientation = ORIENT_AUTO;
  printer->unit = 0;
  printer->v = stp_allocate_vars();
  printer->invalid_mask = INVALID_TOP | INVALID_LEFT;
}

void
copy_printer(gp_plist_t *vd, const gp_plist_t *vs)
{
  if (vs == vd)
    return;
  stp_copy_vars(vd->v, vs->v);
  vd->active = vs->active;
  vd->scaling = vs->scaling;
  vd->orientation = vs->orientation;
  vd->unit = vs->unit;
  vd->invalid_mask = vs->invalid_mask;
  plist_set_name(vd, plist_get_name(vs));
  plist_set_output_to(vd, plist_get_output_to(vs));
}
  

static void
check_plist(int count)
{
  static int current_plist_size = 0;
  int i;
  if (count <= current_plist_size)
    return;
  else if (current_plist_size == 0)
    {
      current_plist_size = count;
      plist = xmalloc(current_plist_size * sizeof(gp_plist_t));
      for (i = 0; i < current_plist_size; i++)
	{
	  memset(&(plist[i]), 0, sizeof(gp_plist_t));
	  initialize_printer(&(plist[i]));
	}
    }
  else
    {
      int old_plist_size = current_plist_size;
      current_plist_size *= 2;
      if (current_plist_size < count)
	current_plist_size = count;
      plist = realloc(plist, current_plist_size * sizeof(gp_plist_t));
      for (i = old_plist_size; i < current_plist_size; i++)
	{
	  memset(&(plist[i]), 0, sizeof(gp_plist_t));
	  initialize_printer(&(plist[i]));
	}
    }
}

/*
 * 'main()' - Main entry - just call gimp_main()...
 */

MAIN()

static int print_finished = 0;

/*
 * 'query()' - Respond to a plug-in query...
 */

static void
query (void)
{
  static /* const */ GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "run_mode",	(BAD_CONST_CHAR) "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,	(BAD_CONST_CHAR) "image",	(BAD_CONST_CHAR) "Input image" },
    { GIMP_PDB_DRAWABLE,	(BAD_CONST_CHAR) "drawable",	(BAD_CONST_CHAR) "Input drawable" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "output_to",	(BAD_CONST_CHAR) "Print command or filename (| to pipe to command)" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "driver",	(BAD_CONST_CHAR) "Printer driver short name" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "ppd_file",	(BAD_CONST_CHAR) "PPD file" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "output_type",	(BAD_CONST_CHAR) "Output type (0 = gray, 1 = color)" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "resolution",	(BAD_CONST_CHAR) "Resolution (\"300\", \"720\", etc.)" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "media_size",	(BAD_CONST_CHAR) "Media size (\"Letter\", \"A4\", etc.)" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "media_type",	(BAD_CONST_CHAR) "Media type (\"Plain\", \"Glossy\", etc.)" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "media_source",	(BAD_CONST_CHAR) "Media source (\"Tray1\", \"Manual\", etc.)" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "brightness",	(BAD_CONST_CHAR) "Brightness (0-400%)" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "scaling",	(BAD_CONST_CHAR) "Output scaling (0-100%, -PPI)" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "orientation",	(BAD_CONST_CHAR) "Output orientation (-1 = auto, 0 = portrait, 1 = landscape)" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "left",	(BAD_CONST_CHAR) "Left offset (points, -1 = centered)" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "top",		(BAD_CONST_CHAR) "Top offset (points, -1 = centered)" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "gamma",	(BAD_CONST_CHAR) "Output gamma (0.1 - 3.0)" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "contrast",	(BAD_CONST_CHAR) "Contrast" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "cyan",	(BAD_CONST_CHAR) "Cyan level" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "magenta",	(BAD_CONST_CHAR) "Magenta level" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "yellow",	(BAD_CONST_CHAR) "Yellow level" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "linear",	(BAD_CONST_CHAR) "Linear output (0 = normal, 1 = linear)" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "image_type",	(BAD_CONST_CHAR) "Image type (0 = line art, 1 = solid tones, 2 = continuous tone, 3 = monochrome)"},
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "saturation",	(BAD_CONST_CHAR) "Saturation (0-1000%)" },
    { GIMP_PDB_FLOAT,	(BAD_CONST_CHAR) "density",	(BAD_CONST_CHAR) "Density (0-200%)" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "ink_type",	(BAD_CONST_CHAR) "Type of ink or cartridge" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "dither_algorithm", (BAD_CONST_CHAR) "Dither algorithm" },
    { GIMP_PDB_INT32,	(BAD_CONST_CHAR) "unit",	(BAD_CONST_CHAR) "Unit 0=Inches 1=Metric" },
  };
  static gint nargs = sizeof(args) / sizeof(args[0]);

  static const gchar *blurb = "This plug-in prints images from The GIMP.";
  static const gchar *help  = "Prints images to PostScript, PCL, or ESC/P2 printers.";
  static const gchar *auth  = "Michael Sweet <mike@easysw.com> and Robert Krawitz <rlk@alum.mit.edu>";
  static const gchar *copy  = "Copyright 1997-2000 by Michael Sweet and Robert Krawitz";
  static const gchar *types = "RGB*,GRAY*,INDEXED*";

  gimp_plugin_domain_register ((BAD_CONST_CHAR) PACKAGE, (BAD_CONST_CHAR) PACKAGE_LOCALE_DIR);

  gimp_install_procedure ((BAD_CONST_CHAR) "file_print_gimp",
			  (BAD_CONST_CHAR) blurb,
			  (BAD_CONST_CHAR) help,
			  (BAD_CONST_CHAR) auth,
			  (BAD_CONST_CHAR) copy,
			  (BAD_CONST_CHAR) PLUG_IN_VERSION,
			  (BAD_CONST_CHAR) N_("<Image>/File/Print..."),
			  (BAD_CONST_CHAR) types,
			  GIMP_PLUGIN,
			  nargs, 0,
			  args, NULL);
}

/*
 * 'usr1_handler()' - Make a note when we receive SIGUSR1.
 */

static volatile int usr1_interrupt;

static void
usr1_handler (int signal)
{
  usr1_interrupt = 1;
}

static void
gimp_writefunc(void *file, const char *buf, size_t bytes)
{
  FILE *prn = (FILE *)file;
  fwrite(buf, 1, bytes, prn);
}

guchar *
get_thumbnail_data(gint *width, gint *height, gint *bpp)
{
  return gimp_image_get_thumbnail_data(image_ID, width, height, bpp);
}

/*
 * 'run()' - Run the plug-in...
 */

/* #define DEBUG_STARTUP */

#ifdef DEBUG_STARTUP
volatile int SDEBUG = 1;
#endif

static void
run (char   *name,		/* I - Name of print program. */
     int    nparams,		/* I - Number of parameters passed in */
     GimpParam *param,		/* I - Parameter values */
     int    *nreturn_vals,	/* O - Number of return values */
     GimpParam **return_vals)	/* O - Return values */
{
  GimpDrawable	*drawable;	/* Drawable for image */
  GimpRunModeType	 run_mode;	/* Current run mode */
  FILE		*prn = NULL;	/* Print file/command */
  int		 ncolors;	/* Number of colors in colormap */
  GimpParam	*values;	/* Return values */
  gint32         drawable_ID;   /* drawable ID */
  GimpExportReturnType export = GIMP_EXPORT_CANCEL;    /* return value of gimp_export_image() */
  int		ppid = getpid (), /* PID of plugin */
		opid,		/* PID of output process */
		cpid = 0,	/* PID of control/monitor process */
		pipefd[2];	/* Fds of the pipe connecting all the above */
  int		dummy;
  gdouble xres, yres;
#ifdef DEBUG_STARTUP
  while (SDEBUG)
    ;
#endif

 /*
  * Initialise libgimpprint
  */

  stp_init();

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

  initialize_printer(&gimp_vars);
  stp_set_input_color_model(gimp_vars.v, COLOR_MODEL_RGB);
  stp_set_output_color_model(gimp_vars.v, COLOR_MODEL_RGB);
  /*
   * Initialize parameter data...
   */

  current_printer = stp_get_printer_by_index (0);
  run_mode = (GimpRunModeType)param[0].data.d_int32;

  values = g_new (GimpParam, 1);

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;

  *nreturn_vals = 1;
  *return_vals  = values;

  image_ID = param[1].data.d_int32;
  drawable_ID = param[2].data.d_int32;

  image_filename = gimp_image_get_filename (image_ID);
  if (strchr(image_filename, '/'))
    image_filename = strrchr(image_filename, '/') + 1;

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
	  *nreturn_vals = 1;
	  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
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
  set_image_dimensions(drawable->width, drawable->height);
  gimp_image_get_resolution (image_ID, &xres, &yres);
  set_image_resolution(xres, yres);

  /*
   * See how we will run
   */

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*
       * Get information from the dialog...
       */

      if (!do_print_dialog (name))
	goto cleanup;
      copy_printer(&gimp_vars, &(plist[plist_current]));
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*
       * Make sure all the arguments are present...
       */
      if (nparams < 11)
	values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      else
	{
	  plist_set_output_to(&gimp_vars, param[3].data.d_string);
	  stp_set_driver(gimp_vars.v, param[4].data.d_string);
	  stp_set_ppd_file(gimp_vars.v, param[5].data.d_string);
	  stp_set_output_type(gimp_vars.v, param[6].data.d_int32);
	  stp_set_string_parameter(gimp_vars.v, "Resolution", param[7].data.d_string);
	  stp_set_string_parameter(gimp_vars.v, "PageSize", param[8].data.d_string);
	  stp_set_string_parameter(gimp_vars.v, "MediaType", param[9].data.d_string);
	  stp_set_string_parameter(gimp_vars.v, "InputSlot", param[10].data.d_string);

          if (nparams > 11)
	    stp_set_float_parameter(gimp_vars.v, "Brightness", param[11].data.d_float);

          if (nparams > 12)
	    gimp_vars.scaling = param[12].data.d_float;

          if (nparams > 13)
	    gimp_vars.orientation = param[13].data.d_int32;

          if (nparams > 14)
            stp_set_left(gimp_vars.v, param[14].data.d_int32);

          if (nparams > 15)
            stp_set_top(gimp_vars.v, param[15].data.d_int32);

          if (nparams > 16)
            stp_set_float_parameter(gimp_vars.v, "Gamma", param[16].data.d_float);

          if (nparams > 17)
	    stp_set_float_parameter(gimp_vars.v, "Contrast", param[17].data.d_float);

          if (nparams > 18)
	    stp_set_float_parameter(gimp_vars.v, "Cyan", param[18].data.d_float);

          if (nparams > 19)
	    stp_set_float_parameter(gimp_vars.v, "Magenta", param[19].data.d_float);

          if (nparams > 20)
	    stp_set_float_parameter(gimp_vars.v, "Yellow", param[20].data.d_float);

          if (nparams > 21)
            stp_set_image_type(gimp_vars.v, param[22].data.d_int32);

          if (nparams > 22)
            stp_set_float_parameter(gimp_vars.v, "Saturation", param[23].data.d_float);

          if (nparams > 23)
            stp_set_float_parameter(gimp_vars.v, "Density", param[24].data.d_float);

	  if (nparams > 24)
	    stp_set_string_parameter(gimp_vars.v, "InkType", param[25].data.d_string);

	  if (nparams > 25)
	    stp_set_string_parameter(gimp_vars.v, "DitherAlgorithm",
			      param[26].data.d_string);

          if (nparams > 26)
	    gimp_vars.unit = param[27].data.d_int32;
	}

      current_printer = stp_get_printer(gimp_vars.v);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      break;

    default:
      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      break;
    }

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

      /*
       * Open the file/execute the print command...
       */

      if (plist_current > 0)
      {
	/*
	 * The following IPC code is only necessary because the GIMP kills
	 * plugins with SIGKILL if its "Cancel" button is pressed; this
	 * gives the plugin no chance whatsoever to clean up after itself.
	 */
	usr1_interrupt = 0;
	signal (SIGUSR1, usr1_handler);
	if (pipe (pipefd) != 0) {
	  prn = NULL;
	} else {
	  cpid = fork ();
	  if (cpid < 0) {
	    prn = NULL;
	  } else if (cpid == 0) {
	    /* LPR monitor process.  Printer output is piped to us. */
	    opid = fork ();
	    if (opid < 0) {
	      /* Errors will cause the plugin to get a SIGPIPE.  */
	      exit (1);
	    } else if (opid == 0) {
	      dup2 (pipefd[0], 0);
	      close (pipefd[0]);
	      close (pipefd[1]);
	      execl("/bin/sh", "/bin/sh", "-c",plist_get_output_to(&gimp_vars),
		    NULL);
	      /* NOTREACHED */
	      exit (1);
	    } else {
	      /*
	       * If the print plugin gets SIGKILLed by gimp, we kill lpr
	       * in turn.  If the plugin signals us with SIGUSR1 that it's
	       * finished printing normally, we close our end of the pipe,
	       * and go away.
	       */
	      close (pipefd[0]);
	      while (usr1_interrupt == 0) {
	        if (kill (ppid, 0) < 0) {
		  /* The print plugin has been killed!  */
		  kill (opid, SIGTERM);
		  waitpid (opid, &dummy, 0);
		  close (pipefd[1]);
		  /*
		   * We do not want to allow cleanup before exiting.
		   * The exiting parent has already closed the connection
		   * to the X server; if we try to clean up, we'll notice
		   * that fact and complain.
		   */
		  _exit (0);
	        }
	        sleep (5);
	      }
	      /* We got SIGUSR1.  */
	      close (pipefd[1]);
	      /*
	       * We do not want to allow cleanup before exiting.
	       * The exiting parent has already closed the connection
	       * to the X server; if we try to clean up, we'll notice
	       * that fact and complain.
	       */
	      _exit (0);
	    }
	  } else {
	    close (pipefd[0]);
	    /* Parent process.  We generate the printer output. */
	    prn = fdopen (pipefd[1], "w");
	    /* and fall through... */
	  }
	}
      }
      else
	prn = fopen (plist_get_output_to(&gimp_vars), "wb");

      if (prn != NULL)
	{
	  int orientation;
	  stp_image_t *image = Image_GimpDrawable_new(drawable);
	  stp_set_float_parameter(gimp_vars.v, "AppGamma", gimp_gamma());
	  stp_merge_printvars(gimp_vars.v,
			      stp_printer_get_printvars(current_printer));

	  /*
	   * Is the image an Indexed type?  If so we need the colormap...
	   */

	  if (gimp_image_base_type (image_ID) == GIMP_INDEXED)
	    stp_set_cmap(gimp_vars.v, gimp_image_get_cmap(image_ID, &ncolors));
	  else
	    stp_set_cmap(gimp_vars.v, NULL);

	  /*
	   * Set up the orientation
	   */
	  orientation = gimp_vars.orientation;
	  if (orientation == ORIENT_AUTO)
	    orientation = compute_orientation();
	  switch (orientation)
	    {
	    case ORIENT_PORTRAIT:
	      break;
	    case ORIENT_LANDSCAPE:
	      Image_rotate_cw(image);
	      break;
	    case ORIENT_UPSIDEDOWN:
	      Image_rotate_180(image);
	      break;
	    case ORIENT_SEASCAPE:
	      Image_rotate_ccw(image);
	      break;
	    }

	  /*
	   * Finally, call the print driver to send the image to the printer
	   * and close the output file/command...
	   */

	  stp_set_outfunc(gimp_vars.v, gimp_writefunc);
	  stp_set_errfunc(gimp_vars.v, gimp_writefunc);
	  stp_set_outdata(gimp_vars.v, prn);
	  stp_set_errdata(gimp_vars.v, stderr);
	  if (stp_print(gimp_vars.v, image) != 1)
	    values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

	  if (plist_current > 0)
	  {
	    fclose (prn);
	    kill (cpid, SIGUSR1);
	    waitpid (cpid, &dummy, 0);
	  }
	  else
	    fclose (prn);
	  print_finished = 1;
	}
      else
	values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

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

 cleanup:
  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image_ID);
  stp_free_vars(gimp_vars.v);
}

/*
 * 'do_print_dialog()' - Pop up the print dialog...
 */

static gint
do_print_dialog (gchar *proc_name)
{

  /*
   * Get printrc options...
   */
  printrc_load ();

  /*
   * Print dialog window...
   */
  create_main_window();

  gtk_main ();
  gdk_flush ();

  /*
   * Set printrc options...
   */
  if (saveme)
    printrc_save ();

  /*
   * Return ok/cancel...
   */
  return (runme);
}

#define GET_MANDATORY_INTERNAL_STRING_PARAM(param)		\
do {								\
  if ((commaptr = strchr(lineptr, ',')) == NULL)		\
    continue;							\
  plist_set_##param##_n(&key, lineptr, commaptr - line);	\
  lineptr = commaptr + 1;					\
} while (0)

#define GET_MANDATORY_STRING_PARAM(param)		\
do {							\
  if ((commaptr = strchr(lineptr, ',')) == NULL)	\
    continue;						\
  stp_set_##param##_n(key.v, lineptr, commaptr - line);	\
  lineptr = commaptr + 1;				\
} while (0)

static int
get_mandatory_string_param(stp_vars_t v, const char *param, char **lineptr)
{
  char *commaptr = strchr(*lineptr, ',');
  if (commaptr == NULL)
    return 0;
  stp_set_string_parameter_n(v, param, *lineptr, commaptr - *lineptr);
  *lineptr = commaptr + 1;
  return 1;
}

#define GET_MANDATORY_INT_PARAM(param)			\
do {							\
  if ((commaptr = strchr(lineptr, ',')) == NULL)	\
    continue;						\
  stp_set_##param(key.v, atoi(lineptr));		\
  lineptr = commaptr + 1;				\
} while (0)

#define GET_MANDATORY_INTERNAL_INT_PARAM(param)		\
do {							\
  if ((commaptr = strchr(lineptr, ',')) == NULL)	\
    continue;						\
  key.param = atoi(lineptr);				\
  lineptr = commaptr + 1;				\
} while (0)

static void
get_optional_string_param(stp_vars_t v, const char *param,
			  char **lineptr, int *keepgoing)
{
  if (*keepgoing)
    {
      char *commaptr = strchr(*lineptr, ',');
      if (commaptr == NULL)
	{
	  stp_set_string_parameter(v, param, *lineptr);
	  *keepgoing = 0;
	}
      else
	{
	  stp_set_string_parameter_n(v, param, *lineptr, commaptr - *lineptr);
	  *lineptr = commaptr + 1;
	}
    }
}

#define GET_OPTIONAL_INT_PARAM(param)					\
do {									\
  if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))	\
    {									\
      keepgoing = 0;							\
    }									\
  else									\
    {									\
      stp_set_##param(key.v, atoi(lineptr));				\
      lineptr = commaptr + 1;						\
    }									\
} while (0)

#define GET_OPTIONAL_INTERNAL_INT_PARAM(param)				\
do {									\
  if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))	\
    {									\
      keepgoing = 0;							\
    }									\
  else									\
    {									\
      key.param = atoi(lineptr);					\
      lineptr = commaptr + 1;						\
    }									\
} while (0)

#define IGNORE_OPTIONAL_PARAM(param)					\
do {									\
  if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))	\
    {									\
      keepgoing = 0;							\
    }									\
  else									\
    {									\
      lineptr = commaptr + 1;						\
    }									\
} while (0)

static void
get_optional_float_param(stp_vars_t v, const char *param,
			 char **lineptr, int *keepgoing)
{
  if (*keepgoing)
    {
      char *commaptr = strchr(*lineptr, ',');
      if (commaptr == NULL)
	{
	  stp_set_float_parameter(v, param, atof(*lineptr));
	  *keepgoing = 0;
	}
      else
	{
	  stp_set_float_parameter(v, param, atof(*lineptr));
	  *lineptr = commaptr + 1;
	}
    }
}

#define GET_OPTIONAL_INTERNAL_FLOAT_PARAM(param)			\
do {									\
  if ((keepgoing == 0) || ((commaptr = strchr(lineptr, ',')) == NULL))	\
    {									\
      keepgoing = 0;							\
    }									\
  else									\
    {									\
      key.param = atof(lineptr);					\
    }									\
} while (0)

static void *
psearch(const void *key, const void *base, size_t nmemb, size_t size,
	int (*compar)(const void *, const void *))
{
  int i;
  const char *cbase = (const char *) base;
  for (i = 0; i < nmemb; i++)
    {
      if ((*compar)(key, (const void *) cbase) == 0)
	return (void *) cbase;
      cbase += size;
    }
  return NULL;
}

int
add_printer(const gp_plist_t *key, int add_only)
{
  /*
   * The format of the list is the File printer followed by a qsort'ed list
   * of system printers. So, if we want to update the file printer, it is
   * always first in the list, else call psearch.
   */
  gp_plist_t *p;
  if (strcmp(_("File"), key->name) == 0
      && strcmp(plist[0].name, _("File")) == 0)
    {
      if (add_only)
	return 0;
      if (stp_get_printer(key->v))
	{
#ifdef DEBUG
	  printf("Updated File printer directly\n");
#endif
	  p = &plist[0];
	  copy_printer(p, key);
	  p->active = 1;
	}
      return 1;
    }
  else if (stp_get_printer(key->v))
    {
      p = psearch(key, plist + 1, plist_count - 1,
		  sizeof(gp_plist_t),
		  (int (*)(const void *, const void *)) compare_printers);
      if (p == NULL)
	{
#ifdef DEBUG
	  fprintf(stderr, "Adding new printer from printrc file: %s\n",
		  key->name);
#endif
	  check_plist(plist_count + 1);
	  p = plist + plist_count;
	  plist_count++;
	  copy_printer(p, key);
	  p->active = 0;
	}
      else
	{
	  if (add_only)
	    return 0;
#ifdef DEBUG
	  printf("Updating printer %s.\n", key->name);
#endif
	  copy_printer(p, key);
	  p->active = 1;
	}
    }
  return 1;
}

/*
 * 'printrc_load()' - Load the printer resource configuration file.
 */
void
printrc_load(void)
{
  int		i;		/* Looping var */
  FILE		*fp;		/* Printrc file */
  char		*filename;	/* Its name */
  char		line[1024],	/* Line in printrc file */
		*lineptr,	/* Pointer in line */
		*commaptr;	/* Pointer to next comma */
  gp_plist_t	key;		/* Search key */
  int		format = 0;	/* rc file format version */
  int		system_printers; /* printer count before reading printrc */
  char *	current_printer = 0; /* printer to select */

  check_plist(1);

 /*
  * Get the printer list...
  */

  get_system_printers();

  system_printers = plist_count - 1;

 /*
  * Generate the filename for the current user...
  */

  filename = gimp_personal_rc_file ((BAD_CONST_CHAR) "printrc");

  if ((fp = fopen(filename, "r")) != NULL)
  {
   /*
    * File exists - read the contents and update the printer list...
    */

    (void) memset(&key, 0, sizeof(gp_plist_t));
    initialize_printer(&key);
    strcpy(key.name, _("File"));
    (void) memset(line, 0, 1024);
    while (fgets(line, sizeof(line), fp) != NULL)
    {
      int keepgoing = 1;
      if (line[0] == '#')
      {
	if (strncmp("#PRINTRCv", line, 9) == 0)
	{
#ifdef DEBUG
          printf("Found printrc version tag: `%s'\n", line);
          printf("Version number: `%s'\n", &(line[9]));
#endif
	  (void) sscanf(&(line[9]), "%d", &format);
	}
        continue;	/* Comment */
      }
      if (format == 0)
      {
       /*
	* Read old format printrc lines...
	*/

        initialize_printer(&key);
	key.invalid_mask = 0;
        lineptr = line;

       /*
        * Read the command-delimited printer definition data.  Note that
        * we can't use sscanf because %[^,] fails if the string is empty...
        */

        GET_MANDATORY_INTERNAL_STRING_PARAM(name);
        GET_MANDATORY_INTERNAL_STRING_PARAM(output_to);
        GET_MANDATORY_STRING_PARAM(driver);

        if (! stp_get_printer(key.v))
	  continue;

        GET_MANDATORY_STRING_PARAM(ppd_file);
        GET_MANDATORY_INT_PARAM(output_type);
	if (!get_mandatory_string_param(key.v, "Resolution", &lineptr))
	  continue;
	if (!get_mandatory_string_param(key.v, "PageSize", &lineptr))
	  continue;
	if (!get_mandatory_string_param(key.v, "MediaType", &lineptr))
	  continue;

	get_optional_string_param(key.v, "InputSlot", &lineptr, &keepgoing);
	get_optional_float_param(key.v, "Brightness", &lineptr, &keepgoing);
	
        GET_OPTIONAL_INTERNAL_FLOAT_PARAM(scaling);
        GET_OPTIONAL_INTERNAL_INT_PARAM(orientation);
        GET_OPTIONAL_INT_PARAM(left);
        GET_OPTIONAL_INT_PARAM(top);
	get_optional_float_param(key.v, "Gamma", &lineptr, &keepgoing);
	get_optional_float_param(key.v, "Contrast", &lineptr, &keepgoing);
	get_optional_float_param(key.v, "Cyan", &lineptr, &keepgoing);
	get_optional_float_param(key.v, "Magenta", &lineptr, &keepgoing);
	get_optional_float_param(key.v, "Yellow", &lineptr, &keepgoing);
        IGNORE_OPTIONAL_PARAM(linear);
        GET_OPTIONAL_INT_PARAM(image_type);
	get_optional_float_param(key.v, "Saturation", &lineptr, &keepgoing);
	get_optional_float_param(key.v, "Density", &lineptr, &keepgoing);
	get_optional_string_param(key.v, "InkType", &lineptr, &keepgoing);
	get_optional_string_param(key.v,"DitherAlgorithm",&lineptr,&keepgoing);
        GET_OPTIONAL_INTERNAL_INT_PARAM(unit);
	add_printer(&key, 0);
      }
      else if (format == 1)
      {
       /*
	* Read new format printrc lines...
	*/

	char *keyword, *end, *value;

	keyword = line;
	for (keyword = line; isspace(*keyword); keyword++)
	{
	  /* skip initial spaces... */
	}
	if (!isalpha(*keyword))
	  continue;
	for (end = keyword; isalnum(*end) || *end == '-'; end++)
	{
	  /* find end of keyword... */
	}
	value = end;
	while (isspace(*value)) {
	  /* skip over white space... */
	  value++;
	}
	if (*value != ':')
	  continue;
	value++;
	*end = '\0';
	while (isspace(*value)) {
	  /* skip over white space... */
	  value++;
	}
	for (end = value; *end && *end != '\n'; end++)
	{
	  /* find end of line... */
	}
	*end = '\0';
#ifdef DEBUG
        printf("Keyword = `%s', value = `%s'\n", keyword, value);
#endif
	if (strcasecmp("current-printer", keyword) == 0) {
	  if (current_printer)
	    free (current_printer);
	  current_printer = g_strdup(value);
	} else if (strcasecmp("printer", keyword) == 0) {
	  /* Switch to printer named VALUE */
	  add_printer(&key, 0);
#ifdef DEBUG
	  printf("output_to is now %s\n", stp_get_output_to(p->v));
#endif

	  initialize_printer(&key);
	  key.invalid_mask = 0;
	  plist_set_name(&key, value);
	} else if (strcasecmp("destination", keyword) == 0) {
	  plist_set_output_to(&key, value);
	} else if (strcasecmp("driver", keyword) == 0) {
	  stp_set_driver(key.v, value);
	} else if (strcasecmp("ppd-file", keyword) == 0) {
	  stp_set_ppd_file(key.v, value);
	} else if (strcasecmp("output-type", keyword) == 0) {
	  stp_set_output_type(key.v, atoi(value));
	} else if (strcasecmp("media-size", keyword) == 0) {
	  stp_set_string_parameter(key.v, "PageSize", value);
	} else if (strcasecmp("media-type", keyword) == 0) {
	  stp_set_string_parameter(key.v, "MediaType", value);
	} else if (strcasecmp("media-source", keyword) == 0) {
	  stp_set_float_parameter(key.v, "Brightness", atof(value));
	} else if (strcasecmp("scaling", keyword) == 0) {
	  key.scaling = atof(value);
	} else if (strcasecmp("orientation", keyword) == 0) {
	  key.orientation = atoi(value);
	} else if (strcasecmp("left", keyword) == 0) {
	  stp_set_left(key.v, atoi(value));
	} else if (strcasecmp("top", keyword) == 0) {
	  stp_set_top(key.v, atoi(value));
	} else if (strcasecmp("linear", keyword) == 0) {
	  /* Ignore linear */
	} else if (strcasecmp("image-type", keyword) == 0) {
	  stp_set_image_type(key.v, atoi(value));
	} else if (strcasecmp("unit", keyword) == 0) {
	  key.unit = atoi(value);
	} else if (strcasecmp("custom-page-width", keyword) == 0) {
	  stp_set_page_width(key.v, atoi(value));
	} else if (strcasecmp("custom-page-height", keyword) == 0) {
	  stp_set_page_height(key.v, atoi(value));
	} else {
	  stp_parameter_t desc;
	  stp_describe_parameter(key.v, keyword, &desc);
	  switch (desc.type)
	    {
	    case STP_PARAMETER_TYPE_STRING_LIST:
	    case STP_PARAMETER_TYPE_FILE:
	      stp_set_string_parameter(key.v, keyword, value);
	      break;
	    case STP_PARAMETER_TYPE_DOUBLE:
	      stp_set_float_parameter(key.v, keyword, atof(value));
	      break;
	    default:
	      if (strlen(value))
		printf("Unrecognized keyword `%s' in printrc; value `%s' (%d)\n",
		       keyword, value, desc.type);
	    }
	}
      }
      else
      {
       /*
        * We cannot read this file format...
        */
      }
    }
    if (format > 0)
      add_printer(&key, 0);
    fclose(fp);
  }

  g_free (filename);

 /*
  * Select the current printer as necessary...
  */

  if (format == 1)
  {
    if (current_printer)
    {
      for (i = 0; i < plist_count; i ++)
        if (strcmp(current_printer, plist[i].name) == 0)
	  plist_current = i;
    }
  }
  else
  {
    if (plist_get_output_to(&gimp_vars)[0] != '\0')
    {
      for (i = 0; i < plist_count; i ++)
	if (strcmp(plist_get_output_to(&gimp_vars),
		   plist_get_output_to(&(plist[i]))) == 0)
          break;

      if (i < plist_count)
        plist_current = i;
    }
  }
}

/*
 * 'printrc_save()' - Save the current printer resource configuration.
 */
void
printrc_save(void)
{
  FILE		*fp;		/* Printrc file */
  char	       *filename;	/* Printrc filename */
  int		i;		/* Looping var */
  gp_plist_t	*p;		/* Current printer */

 /*
  * Generate the filename for the current user...
  */

  filename = gimp_personal_rc_file ((BAD_CONST_CHAR) "printrc");

  if ((fp = fopen(filename, "w")) != NULL)
  {
   /*
    * Write the contents of the printer list...
    */

#ifdef DEBUG
    fprintf(stderr, "Number of printers: %d\n", plist_count);
#endif

    fputs("#PRINTRCv1 written by GIMP-PRINT " PLUG_IN_VERSION "\n", fp);

    fprintf(fp, "Current-Printer: %s\n", plist[plist_current].name);

    for (i = 0, p = plist; i < plist_count; i ++, p ++)
      {
	int count;
	int j;
	const stp_curve_t curve;
	const stp_parameter_t *params = stp_list_parameters(p->v, &count);
	fprintf(fp, "\nPrinter: %s\n", p->name);
	fprintf(fp, "Destination: %s\n", plist_get_output_to(p));
	fprintf(fp, "Scaling: %.3f\n", p->scaling);
	fprintf(fp, "Orientation: %d\n", p->orientation);
	fprintf(fp, "Unit: %d\n", p->unit);

	fprintf(fp, "Driver: %s\n", stp_get_driver(p->v));
	fprintf(fp, "PPD-File: %s\n", stp_get_ppd_file(p->v));

	fprintf(fp, "Left: %d\n", stp_get_left(p->v));
	fprintf(fp, "Top: %d\n", stp_get_top(p->v));
	fprintf(fp, "Custom-Page-Width: %d\n", stp_get_page_width(p->v));
	fprintf(fp, "Custom-Page-Height: %d\n", stp_get_page_height(p->v));

	fprintf(fp, "Output-Type: %d\n", stp_get_output_type(p->v));
	fprintf(fp, "Image-Type: %d\n", stp_get_image_type(p->v));

	for (j = 0; j < count; j++)
	  {
	    if (strcmp(params[j].name, "AppGamma") == 0)
	      continue;
	    switch (params[j].type)
	      {
	      case STP_PARAMETER_TYPE_STRING_LIST:
	      case STP_PARAMETER_TYPE_FILE:
		fprintf(fp, "%s: %s\n", params[j].name,
			stp_get_string_parameter(p->v, params[j].name));
		break;
	      case STP_PARAMETER_TYPE_DOUBLE:
		fprintf(fp, "%s: %f\n", params[j].name,
			stp_get_float_parameter(p->v, params[j].name));
		break;
	      case STP_PARAMETER_TYPE_CURVE:
		curve = stp_get_curve_parameter(p->v, params[j].name);
		stp_curve_print(fp, curve);
		fprintf(fp, "\n");
		break;
	      default:
		break;
	      }
	  }
#ifdef DEBUG
        fprintf(stderr, "Wrote printer %d: %s\n", i, p->name);
#endif
      }
    fclose(fp);
  } else {
    fprintf(stderr,"could not open printrc file \"%s\"\n",filename);
  }
  g_free (filename);
}

/*
 * 'compare_printers()' - Compare system printer names for qsort().
 */

static int
compare_printers(gp_plist_t *p1,	/* I - First printer to compare */
                 gp_plist_t *p2)	/* I - Second printer to compare */
{
  return (strcmp(p1->name, p2->name));
}

/*
 * 'get_system_printers()' - Get a complete list of printers from the spooler.
 */

#define PRINTERS_NONE	0
#define PRINTERS_LPC	1
#define PRINTERS_LPSTAT	2

static void
get_system_printers(void)
{
  int   i;			/* Looping var */
  int	type;			/* 0 = none, 1 = lpc, 2 = lpstat */
  char	command[255];		/* Command to run */
  char  defname[128];		/* Default printer name */
  FILE *pfile;			/* Pipe to status command */
  char  line[255];		/* Line from status command */
  char	*ptr;			/* Pointer into line */
  char  name[128];		/* Printer name from status command */
  static const char	*lpcs[] =	/* Possible locations of LPC... */
		{
		  "/etc"
		  "/usr/bsd",
		  "/usr/etc",
		  "/usr/libexec",
		  "/usr/sbin"
		};

 /*
  * Setup defaults...
  */

  defname[0] = '\0';

  check_plist(1);
  plist_count = 1;
  initialize_printer(&plist[0]);
  strcpy(plist[0].name, _("File"));
  stp_set_driver(plist[0].v, "ps2");
  stp_set_output_type(plist[0].v, OUTPUT_COLOR);

 /*
  * Figure out what command to run...  We use lpstat if it is available over
  * lpc since Solaris, CUPS, etc. provide both commands.  No need to list
  * each printer twice...
  */

  if (!access("/usr/bin/lpstat", X_OK))
  {
    strcpy(command, "/usr/bin/lpstat -d -p");
    type = PRINTERS_LPSTAT;
  }
  else
  {
    for (i = 0; i < (sizeof(lpcs) / sizeof(lpcs[0])); i ++)
    {
      sprintf(command, "%s/lpc", lpcs[i]);

      if (!access(command, X_OK))
        break;
    }

    if (i < (sizeof(lpcs) / sizeof(lpcs[0])))
    {
      strcat(command, " status < /dev/null");
      type = PRINTERS_LPC;
    }
    else
      type = PRINTERS_NONE;
  }

 /*
  * Run the command, if any, to get the available printers...
  */

  if (type > PRINTERS_NONE)
  {
    if ((pfile = popen(command, "r")) != NULL)
    {
     /*
      * Read input as needed...
      */

      while (fgets(line, sizeof(line), pfile) != NULL)
        switch (type)
	{
	  char *result;
	  case PRINTERS_LPC :
	      if (!strncmp(line, "Press RETURN to continue", 24) &&
		  (ptr = strchr(line, ':')) != NULL &&
		  (strlen(ptr) - 2) < (ptr - line))
		strcpy(line, ptr + 2);

	      if ((ptr = strchr(line, ':')) != NULL &&
	          line[0] != ' ' && line[0] != '\t')
              {
		int printer_exists = 0;
		*ptr = '\0';
                /* check for duplicate printers--yes, they can happen,
                 * and it makes gimp-print forget everything about the
                 * printer */
                for (i = 1; i < plist_count; i++)
                  if (strcmp(line, plist[i].name) == 0)
		    {
		      printer_exists = 1;
		      break;
		    }
		if (printer_exists)
		  break;

		check_plist(plist_count + 1);
		initialize_printer(&plist[plist_count]);
		plist_set_name(&(plist[plist_count]), line);
#ifdef DEBUG
                fprintf(stderr, "Adding new printer from lpc: <%s>\n",
                  line);
#endif
		result = g_strdup_printf("lpr -P%s -l", line);
		plist_set_output_to(&(plist[plist_count]), result);
		free(result);
		stp_set_driver(plist[plist_count].v, "ps2");
		plist_count ++;
	      }
	      break;

	  case PRINTERS_LPSTAT :
	      if ((sscanf(line, "printer %127s", name) == 1) ||
		  (sscanf(line, "Printer: %127s", name) == 1))
	      {
		int printer_exists = 0;
                /* check for duplicate printers--yes, they can happen,
                 * and it makes gimp-print forget everything about the
                 * printer */
                for (i = 1; i < plist_count; i++)
                  if (strcmp(name, plist[i].name) == 0)
		    {
		      printer_exists = 1;
		      break;
		    }
		if (printer_exists)
		  break;
		check_plist(plist_count + 1);
		initialize_printer(&plist[plist_count]);
		plist_set_name(&(plist[plist_count]), name);
#ifdef DEBUG
                fprintf(stderr, "Adding new printer from lpc: <%s>\n",
                  name);
#endif
		result = g_strdup_printf("lp -s -d%s -oraw", name);
		plist_set_output_to(&(plist[plist_count]), result);
		free(result);
		stp_set_driver(plist[plist_count].v, "ps2");
        	plist_count ++;
	      }
	      else
        	sscanf(line, "system default destination: %127s", defname);
	      break;
	}

      pclose(pfile);
    }
  }

  if (plist_count > 2)
    qsort(plist + 1, plist_count - 1, sizeof(gp_plist_t),
          (int (*)(const void *, const void *))compare_printers);

  if (defname[0] != '\0' && plist_get_output_to(&gimp_vars)[0] == '\0')
  {
    for (i = 0; i < plist_count; i ++)
      if (strcmp(defname, plist[i].name) == 0)
        break;

    if (i < plist_count)
      plist_current = i;
  }
}

/*
 * End of "$Id$".
 */
