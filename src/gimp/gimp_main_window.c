/*
 * "$Id$"
 *
 *   Main window code for Print plug-in for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu), Steve Miller (smiller@rni.net)
 *      and Michael Natterer (mitch@gimp.org)
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

#define MAX_PREVIEW_PPI        (40)
#define INCH 72
#define FINCH ((gdouble) INCH)
#define ROUNDUP(x, y) (((x) + ((y) - 1)) / (y))
#define SCALE(x, y) (((x) + (1.0 / (2.0 * (y)))) * (y))

#include "print_gimp.h"

#include "print-intl.h"
#include <string.h>

/*
 * Constants for GUI.
 */
#define PREVIEW_SIZE_VERT  360
#define PREVIEW_SIZE_HORIZ 260
#define MINIMUM_IMAGE_PERCENT (5)
#define MOVE_CONSTRAIN	   0
#define MOVE_HORIZONTAL	   1
#define MOVE_VERTICAL      2
#define MOVE_ANY           (MOVE_HORIZONTAL | MOVE_VERTICAL)

/*
 *  Main window widgets
 */

static GtkWidget *main_vbox;
static GtkWidget *main_hbox;
static GtkWidget *right_vbox;
static GtkWidget *notebook;

static GtkWidget *print_dialog;           /* Print dialog window */
static GtkWidget *recenter_button;
static GtkWidget *recenter_vertical_button;
static GtkWidget *recenter_horizontal_button;
static GtkWidget *left_entry;
static GtkWidget *right_entry;
static GtkWidget *right_border_entry;
static GtkWidget *width_entry;
static GtkWidget *top_entry;
static GtkWidget *bottom_entry;
static GtkWidget *bottom_border_entry;
static GtkWidget *height_entry;
static GtkWidget *custom_size_width        = NULL;
static GtkWidget *custom_size_height       = NULL;
static GtkWidget *orientation_menu         = NULL;  /* Orientation menu */
static GtkWidget *scaling_percent;        /* Scale by percent */
static GtkWidget *scaling_ppi;            /* Scale by pixels-per-inch */
static GtkWidget *scaling_image;          /* Scale to the image */
static GtkWidget *setup_dialog;         /* Setup dialog window */
static GtkWidget *printer_driver;       /* Printer driver widget */
static GtkWidget *printer_model_label; /* Printer model name */
static GtkWidget *printer_crawler;      /* Scrolled Window for menu */
static GtkWidget *printer_combo;	/* Combo for menu */
static gint plist_callback_id	   = -1;
static GtkWidget *ppd_file;             /* PPD file entry */
static GtkWidget *ppd_label;            /* PPD file entry */
static GtkWidget *ppd_button;           /* PPD file browse button */
static GtkWidget *output_cmd;           /* Output command text entry */
static GtkWidget *ppd_browser;          /* File selection dialog for PPD files */
static GtkWidget *new_printer_dialog; /* New printer dialog window */
static GtkWidget *new_printer_entry;  /* New printer text entry */

static GtkWidget *file_browser;         /* FSD for print files */
static GtkWidget *adjust_color_button;
static GtkWidget *about_dialog;

static GtkObject *scaling_adjustment;	/* Adjustment object for scaling */
static gboolean   suppress_scaling_adjustment = FALSE;
static gboolean   suppress_scaling_callback   = FALSE;

static gint   suppress_preview_update = 0;

static gint preview_valid = 0;
static gint frame_valid = 0;
static gint need_exposure = 0;

static GtkDrawingArea *preview = NULL;	/* Preview drawing area widget */
static gint mouse_x;		/* Last mouse X */
static gint mouse_y;		/* Last mouse Y */
static gint orig_top;	/* Previous position */
static gint orig_left;	/* Previous position */
static gint buttons_pressed = 0;
static gint preview_active = 0;
static gint buttons_mask = 0;
static gint move_constraint = 0;
static gint mouse_button = -1;	/* Button being dragged with */
static gint suppress_preview_reset = 0;
static gint physical_orientation = -2; /* Actual orientation */
static gint preview_thumbnail_w = 0;
static gint preview_thumbnail_h = 0;

static gint printable_width;	/* Width of page */
static gint printable_height;	/* Height of page */
static gint print_width;	/* Printed width of image */
static gint print_height;	/* Printed height of image */
static gint left, right;	        /* Imageable area */
static gint top, bottom;
static gint paper_width, paper_height;	/* Physical width */
static gint image_width;
static gint image_height;
static gint image_true_width;
static gint image_true_height;
static gdouble image_xres;
static gdouble image_yres;

static void scaling_update        (GtkAdjustment *adjustment);
static void scaling_callback      (GtkWidget *widget);
static void plist_callback        (GtkWidget *widget, gpointer data);
static void custom_media_size_callback(GtkWidget *widget, gpointer data);
static void combo_callback        (GtkWidget *widget, gpointer data);
static void output_type_callback  (GtkWidget *widget, gpointer data);
static void unit_callback         (GtkWidget *widget, gpointer data);
static void orientation_callback  (GtkWidget *widget, gpointer data);
static void printandsave_callback (void);
static void about_callback        (void);
static void print_callback        (void);
static void save_callback         (void);

static void setup_update          (void);
static void setup_open_callback   (void);
static void setup_ok_callback     (void);
static void new_printer_open_callback   (void);
static void new_printer_ok_callback     (void);
static void ppd_browse_callback   (void);
static void ppd_ok_callback       (void);
static void print_driver_callback (GtkWidget      *widget,
				   gint            row,
				   gint            column,
				   GdkEventButton *event,
				   gpointer        data);

static void file_ok_callback      (void);
static void file_cancel_callback  (void);

static void preview_update              (void);
static void preview_expose              (void);
static void preview_button_callback     (GtkWidget      *widget,
					 GdkEventButton *bevent,
					 gpointer        data);
static void preview_motion_callback     (GtkWidget      *widget,
					 GdkEventMotion *mevent,
					 gpointer        data);
static void position_callback           (GtkWidget      *widget);
static void image_type_callback         (GtkWidget      *widget,
					 gpointer        data);
static void set_media_size(const gchar *new_media_size);

static gdouble preview_ppi = 10;
gp_plist_t *pv;

#define Combo_get_text(combo) \
	(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry)))

typedef struct
{
  const char *name;
  const char *text;
  const char *help;
  gint count;
  stp_param_t *params;
  void (*extra)(const gchar *);
  gint callback_id;
  GtkWidget *combo;
} list_option_t;

static list_option_t the_list_options[] =
  {
    { "MediaType", N_("Media Type:"),
      N_("Type of media you're printing to"),
      0, NULL, NULL, -1 },
    { "PageSize", N_("Media Size:"),
      N_("Size of paper that you wish to print to"),
      0, NULL, set_media_size, -1 },
    { "InputSlot", N_("Media Source:"),
      N_("Source (input slot) of media you're printing to"),
      0, NULL, NULL, -1 },
    { "InkType", N_("Ink Type:"),
      N_("Type of ink in the printer"),
      0, NULL, NULL, -1 },
    { "Resolution", N_("Resolution:"),
      N_("Resolution and quality of the print"),
      0, NULL, NULL, -1 },
  };

static const gint list_option_count = sizeof(the_list_options) / sizeof(list_option_t);

typedef struct
{
  const char *name;
  const char *help;
  gdouble scale;
  GtkWidget *checkbox;
  const char *format;
} unit_t;

static unit_t units[] =
  {
    { N_("Inch"), N_("Set the base unit of measurement to inches"),
      72.0, NULL, "%.2f" },
    { N_("cm"), N_("Set the base unit of measurement to centimetres"),
      72.0 / 2.54, NULL, "%.2f" },
    { N_("Points"), N_("Set the base unit of measurement to points"),
      1.0, NULL, "%.0f" },
    { N_("mm"), N_("Set the base unit of measurement to millimetres"),
      72.0 / 25.4, NULL, "%.1f" },
    { N_("Pica"), N_("Set the base unit of measurement to picas"),
      72.0 / 12.0, NULL, "%.1f" },
  };

static const gint unit_count = sizeof(units) / sizeof(unit_t);

typedef struct
{
  const char *name;
  const char *help;
  gint value;
  GtkWidget *button;
} radio_group_t;

static radio_group_t output_types[] =
  {
    { N_("Color"), N_("Color output"), OUTPUT_COLOR, NULL },
    { N_("Grayscale"),
      N_("Print in shades of gray using black ink"), OUTPUT_GRAY, NULL },
    { N_("Black and White"),
      N_("Print in black and white (no color, and no shades of gray)"),
      OUTPUT_MONOCHROME, NULL }
  };

static const gint output_type_count = sizeof(output_types) / sizeof(radio_group_t);

static radio_group_t image_types[] =
  {
    { N_("Line Art"), N_("Fastest and brightest color for text and line art"),
      IMAGE_LINE_ART, NULL },
    { N_("Solid Colors"),
      N_("Best for images dominated by regions of solid color"),
      IMAGE_SOLID_TONE, NULL },
    { N_("Photograph"),
      N_("Slowest, but most accurate and smoothest color for continuous tone "
	 "images and photographs"), IMAGE_CONTINUOUS, NULL }
  };

static const gint image_type_count = sizeof(image_types) / sizeof(radio_group_t);

static stp_param_t *printer_list = 0;
static int printer_count = 0;

static list_option_t *
get_list_option_by_name(const char *name)
{
  int i;
  for (i = 0; i < list_option_count; i++)
    if (strcmp(name, the_list_options[i].name) == 0)
      return &(the_list_options[i]);
  return NULL;
}

void
set_help_data(GtkWidget *widget, const gchar *tooltip)
{
  gimp_help_set_help_data(widget, tooltip, NULL);
}

static void
enable_help(void)
{
  gimp_help_enable_tooltips();
}

static void
disable_help(void)
{
  gimp_help_disable_tooltips();
}

void
set_adjustment_tooltip (GtkObject *adj, const gchar *tip)
{
  set_help_data (GTK_WIDGET (GIMP_SCALE_ENTRY_SCALE (adj)), tip);
  set_help_data (GTK_WIDGET (GIMP_SCALE_ENTRY_SPINBUTTON (adj)), tip);
}

void
table_attach_aligned(GtkTable *table, gint column, gint row,
		     const gchar *label_text, gfloat xalign, gfloat yalign,
		     GtkWidget *widget, gint colspan, gboolean left_align)
{
  gimp_table_attach_aligned(table, column, row, label_text, xalign, yalign,
			    widget, colspan, left_align);
}

static void
create_new_combo(list_option_t *list_option, GtkWidget *table,
		 int hpos, int vpos)
{
  GtkWidget *event_box = gtk_event_box_new();
  list_option->combo = gtk_combo_new();
  gtk_container_add(GTK_CONTAINER(event_box), list_option->combo);
  gtk_widget_show(list_option->combo);
  set_help_data(event_box, _(list_option->help));
  table_attach_aligned(GTK_TABLE(table), hpos, vpos, _(list_option->text),
		       1.0, 0.5, event_box, 1, TRUE);
}

static const char *
Combo_get_name(GtkWidget   *combo,
               gint         num_options,
	       stp_param_t *options)
{
  gchar *text;
  gint   i;

  if ((text = Combo_get_text(combo)) == NULL)
    return (NULL);

  if (num_options == 0)
    return ((const char *)text);

  for (i = 0; i < num_options; i ++)
    if (strcasecmp(options[i].text, text) == 0)
      return (options[i].name);

  return (NULL);
}

static void
build_printer_combo(void)
{
  int i;
  if (printer_list)
    {
      for (i = 0; i < printer_count; i++)
	{
	  free((void *)printer_list[i].name);
	  free((void *)printer_list[i].text);
	}
      free(printer_list);
    }
  printer_list = malloc(sizeof(stp_param_t) * plist_count);
  for (i = 0; i < plist_count; i++)
    {
      if (plist[i].active)
	{
	  printer_list[i].name = g_strdup(plist[i].name);
	  printer_list[i].text = g_strdup(plist[i].name);
	}
      else
	{
	  printer_list[i].name = malloc(strlen(plist[i].name) + 2);
	  printer_list[i].text = malloc(strlen(plist[i].name) + 2);
	  strcpy((char *)printer_list[i].name + 1, plist[i].name);
	  ((char *)printer_list[i].name)[0] = '*';
	  strcpy((char *)printer_list[i].text + 1, plist[i].name);
	  ((char *)printer_list[i].text)[0] = '*';
	}
    }
  printer_count = plist_count;
  plist_build_combo(printer_combo,
		    printer_count,
		    printer_list,
		    printer_list[plist_current].text,
		    NULL,
		    plist_callback,
		    &plist_callback_id,
		    NULL);
}

static void
create_top_level_structure(void)
{
  gchar *plug_in_name;
  /*
   * Create the main dialog
   */

  plug_in_name = g_strdup_printf (_("%s -- Print v%s"),
                                  image_filename, PLUG_IN_VERSION);

  print_dialog =
    gimp_dialog_new (plug_in_name, "print",
                     gimp_standard_help_func, "filters/print.html",
                     GTK_WIN_POS_MOUSE,
                     FALSE, TRUE, FALSE,

		     _("About"), about_callback,
                     NULL, NULL, NULL, FALSE, FALSE,
                     _("Print and\nSave Settings"), printandsave_callback,
                     NULL, NULL, NULL, FALSE, FALSE,
                     _("Save\nSettings"), save_callback,
                     NULL, NULL, NULL, FALSE, FALSE,
                     _("Print"), print_callback,
                     NULL, NULL, NULL, FALSE, FALSE,
                     _("Cancel"), gtk_widget_destroy,
                     NULL, 1, NULL, FALSE, TRUE,

                     NULL);

  g_free (plug_in_name);

  gtk_signal_connect (GTK_OBJECT (print_dialog), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

  /*
   * Top-level containers
   */

  main_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (print_dialog)->vbox), main_vbox,
                      FALSE, FALSE, 0);
  gtk_widget_show (main_vbox);

  main_hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), main_hbox, FALSE, FALSE, 0);
  gtk_widget_show (main_hbox);

  right_vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_end (GTK_BOX (main_hbox), right_vbox, TRUE, TRUE, 0);
  gtk_widget_show (right_vbox);

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (right_vbox), notebook, FALSE, FALSE, 0);
  gtk_widget_show (notebook);
}

static void
create_preview (void)
{
  GtkWidget *frame;
  GtkWidget *event_box;

  frame = gtk_frame_new (_("Preview"));
  gtk_box_pack_start (GTK_BOX (main_hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  preview = (GtkDrawingArea *) gtk_drawing_area_new ();
  gtk_drawing_area_size (preview, PREVIEW_SIZE_HORIZ + 1,
			 PREVIEW_SIZE_VERT + 1);
  event_box = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (event_box), GTK_WIDGET (preview));
  gtk_container_add (GTK_CONTAINER (frame), event_box);
  gtk_widget_show (event_box);

  gtk_signal_connect (GTK_OBJECT (preview), "expose_event",
                      GTK_SIGNAL_FUNC (preview_expose), NULL);
  gtk_signal_connect (GTK_OBJECT (preview), "button_press_event",
                      GTK_SIGNAL_FUNC (preview_button_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (preview), "button_release_event",
                      GTK_SIGNAL_FUNC (preview_button_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (preview), "motion_notify_event",
                      GTK_SIGNAL_FUNC (preview_motion_callback), NULL);
  gtk_widget_show (GTK_WIDGET (preview));

  set_help_data
    (event_box,
     _("Position the image on the page.\n"
       "Click and drag with the primary button to position the image.\n"
       "Click and drag with the second button to move the image with finer precision; "
       "each unit of motion moves the image one point (1/72\")\n"
       "Click and drag with the third (middle) button to move the image in units of "
       "the image size.\n"
       "Holding down the shift key while clicking and dragging constrains the image to "
       "only horizontal or vertical motion.\n"
       "If you click another button while dragging the mouse, the image will return "
       "to its original position."));

  gtk_widget_set_events (GTK_WIDGET (preview),
                         GDK_EXPOSURE_MASK |
                         GDK_BUTTON_MOTION_MASK |
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK);
}

static GtkWidget *
create_entry(GtkWidget *table, int hpos, int vpos, const char *text,
	     const char *help, GtkSignalFunc callback)
{
  GtkWidget *entry = gtk_entry_new();
  gtk_widget_set_usize(entry, 60, 0);
  table_attach_aligned(GTK_TABLE(table), hpos, vpos, text,
		       1.0, 0.5, entry, 1, TRUE);
  set_help_data(entry, help);
  gtk_signal_connect(GTK_OBJECT(entry), "activate",
		     GTK_SIGNAL_FUNC(callback), NULL);
  return entry;
}

static GtkWidget *
create_positioning_entry(GtkWidget *table, int hpos, int vpos,
			 const char *text, const char *help)
{
  return create_entry
    (table, hpos, vpos, text, help, GTK_SIGNAL_FUNC(position_callback));
}

static GtkWidget *
create_positioning_button(GtkWidget *box, const char *text, const char *help)
{
  GtkWidget *button = gtk_button_new_with_label(_(text));
  gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);
  gtk_widget_show(button);
  set_help_data(button, help);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     GTK_SIGNAL_FUNC(position_callback), NULL);
  return button;
}

static GSList *
create_radio_button(radio_group_t *radio, GSList *group,
		    GtkWidget *table, int hpos, int vpos,
		    GtkSignalFunc callback)
{
  radio->button = gtk_radio_button_new_with_label(group, _(radio->name));
  group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio->button));
  table_attach_aligned(GTK_TABLE(table), hpos, vpos, NULL, 0.5, 0.5,
		       radio->button, 1, FALSE);
  set_help_data(radio->button, _(radio->help));
  gtk_signal_connect(GTK_OBJECT(radio->button), "toggled",
		     GTK_SIGNAL_FUNC(callback), (gpointer) radio->value);
  return group;
}

static void
create_positioning_frame (void)
{
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *box;
  GtkWidget *sep;

  frame = gtk_frame_new (_("Position"));
  gtk_box_pack_start (GTK_BOX (right_vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (7, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  /*
   * Orientation option menu.
   */

  orientation_menu =
    gimp_option_menu_new (FALSE,
			  _("Auto"), orientation_callback,
			  (gpointer) ORIENT_AUTO, NULL, NULL, 0,
			  _("Portrait"), orientation_callback,
			  (gpointer) ORIENT_PORTRAIT, NULL, NULL, 0,
			  _("Landscape"), orientation_callback,
			  (gpointer) ORIENT_LANDSCAPE, NULL, NULL, 0,
			  _("Upside down"), orientation_callback,
			  (gpointer) ORIENT_UPSIDEDOWN, NULL, NULL, 0,
			  _("Seascape"), orientation_callback,
			  (gpointer) ORIENT_SEASCAPE, NULL, NULL, 0,
			  NULL);
  set_help_data (orientation_menu,
		 _("Select the orientation: portrait, landscape, "
		   "upside down, or seascape (upside down landscape)"));
  table_attach_aligned (GTK_TABLE (table), 0, 0, _("Orientation:"), 1.0, 0.5,
			orientation_menu, 3, TRUE);

  sep = gtk_hseparator_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), sep, 0, 4, 1, 2);
  gtk_widget_show (sep);

  /*
   * Position entries
   */

  left_entry = create_positioning_entry
    (table, 0, 2, _("Left:"),
     _("Distance from the left of the paper to the image"));
  top_entry = create_positioning_entry
    (table, 2, 2, _("Top:"),
     _("Distance from the top of the paper to the image"));
  right_entry = create_positioning_entry
    (table, 0, 3, _("Right:"),
     _("Distance from the left of the paper to the right of the image"));
  right_border_entry = create_positioning_entry
    (table, 2, 3, _("Right Border:"),
     _("Distance from the right of the paper to the image"));
  bottom_entry = create_positioning_entry
    (table, 0, 4, _("Bottom:"),
     _("Distance from the top of the paper to bottom of the image"));
  bottom_border_entry = create_positioning_entry
    (table, 2, 4, _("Bottom Border:"),
     _("Distance from the bottom of the paper to the image"));

  /*
   * Center options
   */

  box = gtk_hbox_new (TRUE, 4);
  table_attach_aligned (GTK_TABLE (table), 0, 6, _("Center:"), 1.0, 0.5,
			box, 3, FALSE);

  recenter_vertical_button =
    create_positioning_button(box, _("Vertically"),
			      _("Center the image vertically on the paper"));
  recenter_button =
    create_positioning_button(box, _("Both"),
			      _("Center the image on the paper"));
  recenter_horizontal_button =
    create_positioning_button(box, _("Horizontally"),
			      _("Center the image horizontally on the paper"));
}

static void
create_printer_dialog (void)
{
  GtkWidget *table;
  GtkWidget *box;
  GtkWidget *label;
  GtkWidget *event_box;
  gint       i;

  setup_dialog = gimp_dialog_new (_("Setup Printer"), "print",
                                  gimp_standard_help_func, "filters/print.html",
                                  GTK_WIN_POS_MOUSE, FALSE, TRUE, FALSE,

                                  _("OK"), setup_ok_callback,
                                  NULL, NULL, NULL, TRUE, FALSE,
                                  _("Cancel"), gtk_widget_hide,
                                  NULL, 1, NULL, FALSE, TRUE,

                                  NULL);

  /*
   * Top-level table for dialog.
   */

  table = gtk_table_new (5, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 8);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 100);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (setup_dialog)->vbox), table,
                      FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*
   * Printer driver option menu.
   */

  label = gtk_label_new (_("Printer Model:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  event_box = gtk_event_box_new ();
  gtk_table_attach (GTK_TABLE (table), event_box, 1, 3, 0, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (event_box);

  set_help_data (event_box, _("Select your printer model"));

  printer_crawler = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (printer_crawler),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (event_box), printer_crawler);
  gtk_widget_show (printer_crawler);

  printer_driver = gtk_clist_new (1);
  gtk_widget_set_usize (printer_driver, 200, 0);
  gtk_clist_set_selection_mode (GTK_CLIST (printer_driver),
                                GTK_SELECTION_SINGLE);
  gtk_container_add (GTK_CONTAINER (printer_crawler), printer_driver);
  gtk_widget_show (printer_driver);

  gtk_signal_connect (GTK_OBJECT (printer_driver), "select_row",
                      GTK_SIGNAL_FUNC (print_driver_callback),
                      NULL);

  for (i = 0; i < stp_known_printers (); i ++)
    {
      stp_printer_t the_printer = stp_get_printer_by_index (i);

      if (strcmp (stp_printer_get_long_name (the_printer), "") != 0)
	{
	  gchar *tmp =
	    g_strdup (gettext (stp_printer_get_long_name (the_printer)));

	  gtk_clist_insert (GTK_CLIST (printer_driver), i, &tmp);
	  gtk_clist_set_row_data (GTK_CLIST (printer_driver), i, (gpointer) i);
	}
    }

  /*
   * PPD file.
   */

  ppd_label = gtk_label_new (_("PPD File:"));
  gtk_misc_set_alignment (GTK_MISC (ppd_label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), ppd_label, 0, 1, 3, 4,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ppd_label);

  box = gtk_hbox_new (FALSE, 8);
  gtk_table_attach (GTK_TABLE (table), box, 1, 2, 3, 4,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (box);

  ppd_file = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (box), ppd_file, TRUE, TRUE, 0);
  gtk_widget_show (ppd_file);

  set_help_data(ppd_file,_("Enter the correct PPD filename for your printer"));

  ppd_button = gtk_button_new_with_label (_("Browse"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (ppd_button)->child), 2, 0);
  gtk_box_pack_start (GTK_BOX (box), ppd_button, FALSE, FALSE, 0);
  gtk_widget_show (ppd_button);

  set_help_data(ppd_button,
		_("Choose the correct PPD filename for your printer"));
  gtk_signal_connect (GTK_OBJECT (ppd_button), "clicked",
                      GTK_SIGNAL_FUNC (ppd_browse_callback),
                      NULL);

  /*
   * Print command.
   */

  label = gtk_label_new (_("Command:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  output_cmd = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), output_cmd, 1, 2, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (output_cmd);

  set_help_data(output_cmd,
		_("Enter the correct command to print to your printer. "
		  "Note: Please do not remove the `-l' or `-oraw' from "
		  "the command string, or printing will probably fail!"));

  /*
   * Output file selection dialog.
   */

  file_browser = gtk_file_selection_new (_("Print To File?"));

  gtk_signal_connect
    (GTK_OBJECT (GTK_FILE_SELECTION (file_browser)->ok_button), "clicked",
     GTK_SIGNAL_FUNC (file_ok_callback),
     NULL);
  gtk_signal_connect
    (GTK_OBJECT (GTK_FILE_SELECTION (file_browser)->cancel_button), "clicked",
     GTK_SIGNAL_FUNC (file_cancel_callback),
     NULL);

  /*
   * PPD file selection dialog.
   */

  ppd_browser = gtk_file_selection_new (_("PPD File?"));
  gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (ppd_browser));

  gtk_signal_connect
    (GTK_OBJECT (GTK_FILE_SELECTION (ppd_browser)->ok_button), "clicked",
     GTK_SIGNAL_FUNC (ppd_ok_callback),
     NULL);
  gtk_signal_connect_object
    (GTK_OBJECT (GTK_FILE_SELECTION (ppd_browser)->cancel_button), "clicked",
     GTK_SIGNAL_FUNC (gtk_widget_hide),
     GTK_OBJECT (ppd_browser));
}

static void
create_new_printer_dialog (void)
{
  GtkWidget *table;

  new_printer_dialog =
    gimp_dialog_new (_("Define New Printer"), "print",
                     gimp_standard_help_func, "filters/print.html",
                     GTK_WIN_POS_MOUSE, FALSE, TRUE, FALSE,

                     _("OK"), new_printer_ok_callback,
		     NULL, NULL, NULL, TRUE, FALSE,
                     _("Cancel"), gtk_widget_hide,
                     NULL, 1, NULL, FALSE, TRUE,

		     NULL);

  table = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 8);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (new_printer_dialog)->vbox), table,
                      FALSE, FALSE, 0);
  gtk_widget_show (table);

  new_printer_entry = gtk_entry_new ();
  gtk_entry_set_max_length (GTK_ENTRY (new_printer_entry), 127);
  table_attach_aligned(GTK_TABLE (table), 0, 0, _("Printer Name:"), 1.0,
		       0.5, new_printer_entry, 1, TRUE);

  set_help_data(new_printer_entry,
		_("Enter the name you wish to give this logical printer"));
  gtk_signal_connect (GTK_OBJECT (new_printer_entry), "activate",
                      GTK_SIGNAL_FUNC (new_printer_ok_callback),
                      NULL);
}

static void
create_about_dialog (void)
{
  GtkWidget *label;
  about_dialog =
    gimp_dialog_new (_("About Gimp-Print " PLUG_IN_VERSION), "print",
                     gimp_standard_help_func, "filters/print.html",
                     GTK_WIN_POS_MOUSE, FALSE, TRUE, FALSE,

                     _("OK"), gtk_widget_hide,
                     NULL, 1, NULL, TRUE, TRUE,

		     NULL);

  label = gtk_label_new
    (_("Gimp-Print Version " PLUG_IN_VERSION "\n"
       "\n"
       "Copyright (C) 1997-2001 Michael Sweet, Robert Krawitz,\n"
       "and the rest of the Gimp-Print Development Team.\n"
       "\n"
       "Please visit our web site at http://gimp-print.sourceforge.net.\n"
       "\n"
       "This program is free software; you can redistribute it and/or modify\n"
       "it under the terms of the GNU General Public License as published by\n"
       "the Free Software Foundation; either version 2 of the License, or\n"
       "(at your option) any later version.\n"
       "\n"
       "This program is distributed in the hope that it will be useful,\n"
       "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
       "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
       "GNU General Public License for more details.\n"
       "\n"
       "You should have received a copy of the GNU General Public License\n"
       "along with this program; if not, write to the Free Software\n"
       "Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  "
       "USA\n"));

  gtk_misc_set_padding (GTK_MISC (label), 12, 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about_dialog)->vbox), label,
                      FALSE, FALSE, 0);
  gtk_widget_show (label);
}

static void
create_printer_settings_frame (void)
{
  GtkWidget *table;
  GtkWidget *printer_hbox;
  GtkWidget *media_size_table;
  GtkWidget *button;
  GtkWidget *event_box;
  gint vpos = 0;
  gint i;

  create_printer_dialog ();
  create_about_dialog ();
  create_new_printer_dialog ();

  table = gtk_table_new (list_option_count + 4, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                            table,
                            gtk_label_new (_("Printer Settings")));
  gtk_widget_show (table);

  /*
   * Printer option menu.
   */

  printer_combo = gtk_combo_new ();
  event_box = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (event_box), printer_combo);
  gtk_widget_show (printer_combo);

  set_help_data(event_box,
		_("Select the name of the printer (not the type, "
		  "or model, of printer) that you wish to print to"));
  table_attach_aligned(GTK_TABLE (table), 0, vpos++, _("Printer Name:"),
		       1.0, 0.5, event_box, 2, TRUE);

  printer_model_label = gtk_label_new ("");
  table_attach_aligned(GTK_TABLE (table), 0, vpos++, _("Printer Model:"),
		       1.0, 0.5, printer_model_label, 2, TRUE);

  printer_hbox = gtk_hbox_new (TRUE, 4);
  gtk_table_attach_defaults (GTK_TABLE (table), printer_hbox,
			     1, 2, vpos, vpos + 1);
  vpos += 2;
  gtk_widget_show (printer_hbox);

  /*
   * Setup printer button
   */

  button = gtk_button_new_with_label (_("Setup Printer..."));
  set_help_data(button, _("Choose the printer model, PPD file, and command "
			  "that is used to print to this printer"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_box_pack_start (GTK_BOX (printer_hbox), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (setup_open_callback),
                      NULL);

  /*
   * New printer button
   */

  button = gtk_button_new_with_label (_("New Printer..."));
  set_help_data (button, _("Define a new logical printer. This can be used to "
			   "name a collection of settings that you wish to "
			   "remember for future use."));
  gtk_box_pack_start (GTK_BOX (printer_hbox), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (new_printer_open_callback),
                      NULL);

  /*
   * Media size combo box.
   */

  create_new_combo(get_list_option_by_name("PageSize"), table, 0, vpos++);

  /*
   * Custom media size entries
   */

  media_size_table = gtk_table_new (1, 4, FALSE);
  table_attach_aligned(GTK_TABLE (table), 0, vpos++, _("Dimensions:"),
		       1.0, 0.5, media_size_table, 2, TRUE);

  custom_size_width = create_entry
    (media_size_table, 0, 2, _("Width:"),
     _("Width of the paper that you wish to print to"),
     custom_media_size_callback);

  custom_size_height = create_entry
    (media_size_table, 2, 2, _("Height:"),
     _("Height of the paper that you wish to print to"),
     custom_media_size_callback);

  for (i = 0; i < list_option_count; i++)
    {
      if (strcmp(the_list_options[i].name, "PageSize"))
	create_new_combo(&(the_list_options[i]), table, 0, vpos++);
    }
}

static void
create_scaling_frame (void)
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *box;
  GtkWidget *label;
  GtkWidget *event_box;
  GtkWidget *sep;
  GSList    *group;
  gint i;

  frame = gtk_frame_new (_("Size"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*
   * Create the scaling adjustment using percent.  It doesn't really matter,
   * since as soon as we call plist_callback at the end of initialization
   * everything will be put right.
   */
  scaling_adjustment =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 0, _("Scaling:"), 100, 75,
			  100.0,
			  (gdouble) MINIMUM_IMAGE_PERCENT,
			  100.0,
			  1.0, 10.0, 1, TRUE, 0, 0, NULL, NULL);
  set_adjustment_tooltip(scaling_adjustment,
			 _("Set the scale (size) of the image"));
  gtk_signal_connect (GTK_OBJECT (scaling_adjustment), "value_changed",
                      GTK_SIGNAL_FUNC (scaling_update),
                      NULL);

  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 0);
  gtk_widget_show (sep);

  box = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);
  gtk_widget_show (box);

  /*
   * The scale by percent/ppi toggles
   */

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (box), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  event_box = gtk_event_box_new ();
  gtk_table_attach (GTK_TABLE (table), event_box, 0, 1, 0, 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (event_box);

  label = gtk_label_new ("Scale by:");
  gtk_container_add (GTK_CONTAINER (event_box), label);
  gtk_widget_show (label);

  set_help_data(event_box,
		_("Select whether scaling is measured as percent of "
		  "available page size or number of output dots per inch"));

  scaling_percent = gtk_radio_button_new_with_label (NULL, _("Percent"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (scaling_percent));
  table_attach_aligned(GTK_TABLE (table), 0, 0, NULL, 0.5, 0.5,
		       scaling_percent, 1, TRUE);

  set_help_data(scaling_percent, _("Scale the print to the size of the page"));
  gtk_signal_connect (GTK_OBJECT (scaling_percent), "toggled",
                      GTK_SIGNAL_FUNC (scaling_callback),
                      NULL);

  scaling_ppi = gtk_radio_button_new_with_label (group, _("PPI"));
  table_attach_aligned(GTK_TABLE (table), 0, 1, NULL, 0.5, 0.5,
		       scaling_ppi, 1, TRUE);

  set_help_data(scaling_ppi,
		_("Scale the print to the number of dots per inch"));
  gtk_signal_connect (GTK_OBJECT (scaling_ppi), "toggled",
                      GTK_SIGNAL_FUNC (scaling_callback),
                      NULL);

  sep = gtk_vseparator_new ();
  gtk_box_pack_start (GTK_BOX (box), sep, FALSE, FALSE, 8);
  gtk_widget_show (sep);

  /*
   * The width/height enries
   */

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (box), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  width_entry = create_positioning_entry
    (table, 0, 0, _("Width:"), _("Set the width of the print"));
  height_entry = create_positioning_entry
    (table, 0, 1, _("Height:"), _("Set the height of the print"));

  /*
   * The units toggles
   */

  table = gtk_table_new (2, (unit_count + 1) / 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (box), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  event_box = gtk_event_box_new ();
  gtk_table_attach (GTK_TABLE (table), event_box, 0, 1, 0, 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (event_box);

  label = gtk_label_new (_("Units:"));
  gtk_container_add (GTK_CONTAINER (event_box), label);
  gtk_widget_show (label);

  set_help_data(event_box,
		_("Select the base unit of measurement for printing"));

  group = NULL;
  for (i = 0; i < unit_count; i++)
    {
      unit_t *unit = &(units[i]);
      unit->checkbox = gtk_radio_button_new_with_label(group, _(unit->name));
      group = gtk_radio_button_group(GTK_RADIO_BUTTON(unit->checkbox));
      table_attach_aligned(GTK_TABLE(table), i / 2, i % 2, NULL, 0.5,
			   0.5, unit->checkbox, 1, TRUE);
      set_help_data(unit->checkbox, _(unit->help));
      gtk_signal_connect(GTK_OBJECT(unit->checkbox), "toggled",
			 GTK_SIGNAL_FUNC(unit_callback), (gpointer) i);
    }

  /*
   * The "image size" button
   */

  scaling_image = gtk_button_new_with_label (_("Use Original\nImage Size"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (scaling_image)->child), 8, 4);
  gtk_box_pack_end (GTK_BOX (box), scaling_image, FALSE, TRUE, 0);
  gtk_widget_show (scaling_image);

  set_help_data(scaling_image,
		_("Set the print size to the size of the image"));
  gtk_signal_connect (GTK_OBJECT (scaling_image), "clicked",
                      GTK_SIGNAL_FUNC (scaling_callback),
                      NULL);

}

static void
create_image_settings_frame (void)
{
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *event_box;
  GtkWidget *sep;
  GSList    *group;
  gint i;

  create_color_adjust_window ();

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                            vbox,
                            gtk_label_new (_("Image / Output Settings")));
  gtk_widget_show (vbox);

  table = gtk_table_new (image_type_count, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  event_box = gtk_event_box_new ();
  gtk_table_attach (GTK_TABLE (table), event_box, 0, 1, 0, 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (event_box);

  label = gtk_label_new (_("Image Type:"));
  gtk_container_add (GTK_CONTAINER (event_box), label);
  gtk_widget_show (label);

  set_help_data(event_box,
		_("Optimize the output for the type of image being printed"));

  group = NULL;
  for (i = 0; i < image_type_count; i++)
    group = create_radio_button(&(image_types[i]), group, table, 0, i,
				image_type_callback);

  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 0);
  gtk_widget_show (sep);

  /*
   * Output type toggles.
   */

  table = gtk_table_new (output_type_count + 1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 2, 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  event_box = gtk_event_box_new ();
  gtk_table_attach (GTK_TABLE (table), event_box, 0, 1, 0, 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (event_box);

  label = gtk_label_new (_("Output Type:"));
  gtk_container_add (GTK_CONTAINER (event_box), label);
  gtk_widget_show (label);

  set_help_data(event_box, _("Select the desired output type"));

  group = NULL;
  for (i = 0; i < output_type_count; i++)
    group = create_radio_button(&(output_types[i]), group, table, 0, i,
				output_type_callback);

  /*
   *  Color adjust button
   */

  adjust_color_button = gtk_button_new_with_label (_("Adjust Output..."));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (adjust_color_button)->child), 4, 0);
  table_attach_aligned(GTK_TABLE (table), 0, output_type_count,
		       NULL, 0.5, 0.5, adjust_color_button, 1, TRUE);

  set_help_data(adjust_color_button,
		_("Adjust color balance, brightness, contrast, "
		  "saturation, and dither algorithm"));
  gtk_signal_connect_object (GTK_OBJECT (adjust_color_button), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_show),
			     GTK_OBJECT (color_adjust_dialog));
}

/*
 *  create_main_window()
 */
void
create_main_window (void)
{

  pv = &(plist[plist_current]);
  /*
   * Create the various dialog components.  Note that we're not
   * actually initializing the values at this point; that will be done after
   * the UI is fully created.
   */
  gimp_help_init ();

  create_top_level_structure ();

  create_preview ();
  create_printer_settings_frame ();
  create_positioning_frame ();
  create_scaling_frame ();
  create_image_settings_frame ();

  /*
   * Now actually set up the correct values in the dialog
   */

  build_printer_combo ();
  plist_callback (NULL, (gpointer) plist_current);
  update_adjusted_thumbnail ();

  gtk_widget_show (print_dialog);
}

static void
set_entry_value(GtkWidget *entry, double value, int block)
{
  gchar s[255];
  gdouble unit_scaler = units[pv->unit].scale;
  const gchar *format = units[pv->unit].format;

  g_snprintf(s, sizeof(s), format, value / unit_scaler);
  if (block)
    gtk_signal_handler_block_by_data (GTK_OBJECT (entry), NULL);
  gtk_entry_set_text (GTK_ENTRY (entry), s);
  if (block)
    gtk_signal_handler_unblock_by_data (GTK_OBJECT (entry), NULL);
}

static void
reset_preview(void)
{
  if (!suppress_preview_reset)
    {
      enable_help();
      buttons_pressed = preview_active = 0;
    }
}

static void
compute_scaling_limits(gdouble *min_ppi_scaling, gdouble *max_ppi_scaling)
{
  gdouble min_ppi_scaling1, min_ppi_scaling2;
  min_ppi_scaling1 = FINCH * (gdouble) image_width / (gdouble) printable_width;
  min_ppi_scaling2 = FINCH * (gdouble)image_height / (gdouble)printable_height;

  if (min_ppi_scaling1 > min_ppi_scaling2)
    *min_ppi_scaling = min_ppi_scaling1;
  else
    *min_ppi_scaling = min_ppi_scaling2;

  *max_ppi_scaling = *min_ppi_scaling * 100 / (gdouble) MINIMUM_IMAGE_PERCENT;
}

/*
 *  scaling_update() - Update the scaling scale using the slider.
 */
static void
scaling_update (GtkAdjustment *adjustment)
{
  reset_preview ();

  if (pv->scaling != adjustment->value)
    {
      invalidate_preview_thumbnail ();
      if (GTK_TOGGLE_BUTTON (scaling_ppi)->active)
	pv->scaling = -adjustment->value;
      else
	pv->scaling = adjustment->value;

      suppress_scaling_adjustment = TRUE;
      preview_update ();
      suppress_scaling_adjustment = FALSE;
    }
}

/*
 *  scaling_callback() - Update the scaling scale using radio buttons.
 */
static void
scaling_callback (GtkWidget *widget)
{
  gdouble max_ppi_scaling;
  gdouble min_ppi_scaling;
  gdouble current_scale;

  reset_preview ();

  if (suppress_scaling_callback)
    return;

  compute_scaling_limits(&min_ppi_scaling, &max_ppi_scaling);

  if (widget == scaling_ppi)
    {
      if (! GTK_TOGGLE_BUTTON (scaling_ppi)->active)
	return;

      GTK_ADJUSTMENT (scaling_adjustment)->lower = min_ppi_scaling;
      GTK_ADJUSTMENT (scaling_adjustment)->upper = max_ppi_scaling;

      /*
       * Compute the correct PPI to create an image of the same size
       * as the one measured in percent
       */
      current_scale = GTK_ADJUSTMENT (scaling_adjustment)->value;
      GTK_ADJUSTMENT (scaling_adjustment)->value =
	min_ppi_scaling / (current_scale / 100);
      pv->scaling = 0.0;
    }
  else if (widget == scaling_percent)
    {
      gdouble new_percent;

      if (! GTK_TOGGLE_BUTTON (scaling_percent)->active)
	return;

      current_scale = GTK_ADJUSTMENT (scaling_adjustment)->value;
      GTK_ADJUSTMENT (scaling_adjustment)->lower = (gdouble) MINIMUM_IMAGE_PERCENT;
      GTK_ADJUSTMENT (scaling_adjustment)->upper = 100.0;

      new_percent = 100 * min_ppi_scaling / current_scale;

      if (new_percent > 100)
	new_percent = 100;
      if (new_percent < (gdouble) MINIMUM_IMAGE_PERCENT)
	new_percent = (gdouble) MINIMUM_IMAGE_PERCENT;

      GTK_ADJUSTMENT (scaling_adjustment)->value = new_percent;
      pv->scaling = 0.0;
    }
  else if (widget == scaling_image)
    {
      gdouble yres = image_yres;

      invalidate_preview_thumbnail ();

      GTK_ADJUSTMENT (scaling_adjustment)->lower = min_ppi_scaling;
      GTK_ADJUSTMENT (scaling_adjustment)->upper = max_ppi_scaling;

      if (yres < min_ppi_scaling)
	yres = min_ppi_scaling;
      if (yres > max_ppi_scaling)
	yres = max_ppi_scaling;

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scaling_ppi), TRUE);
      GTK_ADJUSTMENT (scaling_adjustment)->value = yres;
      pv->scaling = 0.0;
    }

  if (widget == scaling_ppi || widget == scaling_percent)
    suppress_preview_update++;
  gtk_adjustment_changed (GTK_ADJUSTMENT (scaling_adjustment));
  gtk_adjustment_value_changed (GTK_ADJUSTMENT (scaling_adjustment));
  if (widget == scaling_ppi || widget == scaling_percent)
    suppress_preview_update--;
}

/****************************************************************************
 *
 * plist_build_combo
 *
 ****************************************************************************/
void
plist_build_combo (GtkWidget      *combo,       /* I - Combo widget */
		   gint            num_items,   /* I - Number of items */
		   stp_param_t    *items,       /* I - Menu items */
		   const gchar    *cur_item,    /* I - Current item */
		   const gchar    *def_value,   /* I - default item */
		   GtkSignalFunc   callback,    /* I - Callback */
		   gint           *callback_id, /* IO - Callback ID (init to -1) */
		   gpointer        data)
{
  gint      i; /* Looping var */
  GList    *list = 0;
  GtkEntry *entry = GTK_ENTRY (GTK_COMBO (combo)->entry);

  if (*callback_id != -1)
    gtk_signal_disconnect (GTK_OBJECT (entry), *callback_id);
#if 0
  gtk_signal_handlers_destroy (GTK_OBJECT (entry));
#endif
  gtk_entry_set_editable (entry, FALSE);

  if (num_items == 0)
    {
      list = g_list_append (list, _("Standard"));
      gtk_combo_set_popdown_strings (GTK_COMBO (combo), list);
      *callback_id = -1;
      gtk_widget_set_sensitive (combo, FALSE);
      gtk_widget_show (combo);
      return;
    }

  for (i = 0; i < num_items; i ++)
    list = g_list_append (list, g_strdup(items[i].text));

  gtk_combo_set_popdown_strings (GTK_COMBO (combo), list);

  *callback_id = gtk_signal_connect (GTK_OBJECT (entry), "changed", callback,
				     data);

  for (i = 0; i < num_items; i ++)
    if (strcmp(items[i].name, cur_item) == 0)
      break;

  if (i >= num_items)
    {
      if (def_value)
        for (i = 0; i < num_items; i ++)
          if (strcmp(items[i].name, def_value) == 0)
            break;

      if (i >= num_items)
        i = 0;
    }

  gtk_entry_set_text (entry, g_strdup (items[i].text));

  gtk_combo_set_value_in_list (GTK_COMBO (combo), TRUE, FALSE);
  gtk_widget_set_sensitive (combo, TRUE);
  gtk_widget_show (combo);
}

void
set_image_dimensions(gint width, gint height)
{
  image_true_width = width;
  image_true_height = height;
}

void
set_image_resolution(gdouble xres, gdouble yres)
{
  image_xres = xres;
  image_yres = yres;
}

gint
compute_orientation(void)
{
  if ((printable_width >= printable_height &&
       image_true_width >= image_true_height) ||
      (printable_height >= printable_width &&
       image_true_height >= image_true_width))
    return ORIENT_PORTRAIT;
  else
    return ORIENT_LANDSCAPE;
}

static void
set_orientation(int orientation)
{
  pv->orientation = orientation;
  if (orientation == ORIENT_AUTO)
    orientation = compute_orientation();
  physical_orientation = orientation;
  switch (orientation)
    {
    case ORIENT_PORTRAIT:
    case ORIENT_UPSIDEDOWN:
      image_height = image_true_height;
      image_width = image_true_width;
      preview_thumbnail_h = thumbnail_h;
      preview_thumbnail_w = thumbnail_w;
      break;
    case ORIENT_LANDSCAPE:
    case ORIENT_SEASCAPE:
      image_height = image_true_width;
      image_width = image_true_height;
      preview_thumbnail_h = thumbnail_w;
      preview_thumbnail_w = thumbnail_h;
      break;
    }
  update_adjusted_thumbnail();
}

/*
 *  do_misc_updates() - Build an option menu for the given parameters.
 */
static void
do_misc_updates (void)
{
  gint i;
  suppress_preview_update++;
  set_orientation(pv->orientation);
  invalidate_preview_thumbnail ();
  preview_update ();

  if (pv->scaling < 0)
    {
      gdouble tmp = -pv->scaling;
      gdouble max_ppi_scaling;
      gdouble min_ppi_scaling;

      compute_scaling_limits(&min_ppi_scaling, &max_ppi_scaling);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scaling_ppi), TRUE);
      GTK_ADJUSTMENT (scaling_adjustment)->lower = min_ppi_scaling;
      GTK_ADJUSTMENT (scaling_adjustment)->upper = max_ppi_scaling;
      GTK_ADJUSTMENT (scaling_adjustment)->value = tmp;
      gtk_adjustment_changed (GTK_ADJUSTMENT (scaling_adjustment));
      gtk_adjustment_value_changed (GTK_ADJUSTMENT (scaling_adjustment));
    }
  else
    {
      gdouble tmp = pv->scaling;

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scaling_percent), TRUE);
      GTK_ADJUSTMENT (scaling_adjustment)->lower = (gdouble) MINIMUM_IMAGE_PERCENT;
      GTK_ADJUSTMENT (scaling_adjustment)->upper = 100.0;
      GTK_ADJUSTMENT (scaling_adjustment)->value = tmp;
      gtk_signal_emit_by_name (scaling_adjustment, "changed");
      gtk_signal_emit_by_name (scaling_adjustment, "value_changed");
    }

  for (i = 0; i < output_type_count; i++)
    {
      if (output_types[i].value == stp_get_output_type(pv->v))
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(output_types[i].button),
				     TRUE);
    }

  do_color_updates ();

  gtk_option_menu_set_history (GTK_OPTION_MENU (orientation_menu),
			       pv->orientation + 1);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(units[pv->unit].checkbox),
			       TRUE);

  for (i = 0; i < image_type_count; i++)
    {
      if (image_types[i].value == stp_get_image_type(pv->v))
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(image_types[i].button),
				     TRUE);
    }

  suppress_preview_update--;
  preview_update ();
}

/*
 * position_callback() - callback for position entry widgets
 */
static void
position_callback (GtkWidget *widget)
{
  reset_preview ();
  suppress_preview_update++;

  if (widget == recenter_button)
    {
      pv->left_is_valid = 0;
      pv->top_is_valid = 0;
    }
  else if (widget == recenter_horizontal_button)
    {
      pv->left_is_valid = 0;
    }
  else if (widget == recenter_vertical_button)
    {
      pv->top_is_valid = 0;
    }
  else
    {
      gdouble new_printed_value =
	atof (gtk_entry_get_text (GTK_ENTRY (widget)));
      gdouble unit_scaler = units[pv->unit].scale;
      gint new_value = SCALE(new_printed_value, unit_scaler);
      gboolean was_percent = 0;

      if (widget == top_entry)
	stp_set_top(pv->v, new_value);
      else if (widget == bottom_entry)
	stp_set_top(pv->v, new_value - print_height);
      else if (widget == bottom_border_entry)
	stp_set_top (pv->v, paper_height - print_height - new_value);
      else if (widget == left_entry)
	stp_set_left (pv->v, new_value);
      else if (widget == right_entry)
	stp_set_left(pv->v, new_value - print_width);
      else if (widget == right_border_entry)
	stp_set_left (pv->v, paper_width - print_width - new_value);
      else if (widget == width_entry || widget == height_entry)
	{
	  if (pv->scaling >= 0)
	    {
	      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scaling_ppi),
					    TRUE);
	      scaling_callback (scaling_ppi);
	      was_percent = 1;
	    }
	  if  (widget == width_entry)
	    GTK_ADJUSTMENT (scaling_adjustment)->value =
	      ((gdouble) image_width) / (new_value / FINCH);
	  else
	    GTK_ADJUSTMENT (scaling_adjustment)->value =
	      ((gdouble) image_height) / (new_value / FINCH);
	  gtk_adjustment_value_changed (GTK_ADJUSTMENT (scaling_adjustment));
	  if (was_percent)
	    {
	      gtk_toggle_button_set_active
		(GTK_TOGGLE_BUTTON (scaling_percent), TRUE);
	      gtk_adjustment_value_changed
		(GTK_ADJUSTMENT (scaling_adjustment));
	    }
	}
    }

  suppress_preview_update--;
  preview_update ();
}

/*
 *  plist_callback() - Update the current system printer.
 */
static void
plist_callback (GtkWidget *widget,
		gpointer   data)
{
  gint         i;
  const gchar *default_parameter;

  invalidate_frame ();
  invalidate_preview_thumbnail ();
  reset_preview ();

  if (widget)
    {
      const gchar *result = Combo_get_text (printer_combo);

      for (i = 0; i < plist_count; i++)
	{
	  if (! strcmp (result, printer_list[i].text))
	    {
	      plist_current = i;
	      break;
	    }
	}
    }
  else
    {
      plist_current = (gint) data;
    }

  pv = &(plist[plist_current]);

  if (strcmp(stp_get_driver(pv->v), ""))
    current_printer = stp_get_printer_by_driver (stp_get_driver (pv->v));

  suppress_preview_update++;
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (dither_algo_combo)->entry),
                      stp_get_parameter (pv->v, "DitherAlgorithm"));

  setup_update ();

  do_misc_updates ();

  /*
   * Now get option parameters.
   */

  for (i = 0; i < list_option_count; i++)
    {
      list_option_t *option = &(the_list_options[i]);
      if (option->count > 0)
	{
	  int j;
	  for (j = 0; j < option->count; j++)
	    {
	      free ((void *)(option->params[j].name));
	      free ((void *)(option->params[j].text));
	    }
	  free(option->params);
	  option->count = 0;
	}
      option->params = stp_printer_get_parameters
	(current_printer, pv->v, option->name, &(option->count));
      default_parameter =
	stp_printer_get_default_parameter(current_printer, pv->v,option->name);
      if (stp_get_parameter(pv->v, option->name)[0] == '\0')
	stp_set_parameter(pv->v, option->name, default_parameter);
      else if (option->params == NULL)
	stp_set_parameter(pv->v, option->name, NULL);
      plist_build_combo(option->combo, option->count, option->params,
			stp_get_parameter(pv->v, option->name),
			default_parameter, combo_callback,
			&(option->callback_id), option);
      if (option->extra)
	(option->extra)(stp_get_parameter(pv->v, option->name));
    }

  if (dither_algo_combo)
    build_dither_combo ();

  suppress_preview_update--;
  preview_update ();
}

static void
custom_media_size_callback(GtkWidget *widget,
			   gpointer data)
{
  gint width_limit, height_limit;
  gint min_width_limit, min_height_limit;
  gdouble new_printed_value = atof(gtk_entry_get_text(GTK_ENTRY(widget)));
  gdouble unit_scaler = units[pv->unit].scale;
  gint new_value = SCALE(new_printed_value, unit_scaler);
  invalidate_frame ();
  invalidate_preview_thumbnail ();
  reset_preview ();

  stp_printer_get_size_limit(current_printer, pv->v,
			     &width_limit, &height_limit,
			     &min_width_limit, &min_height_limit);
  if (widget == custom_size_width)
    {
      if (new_value < min_width_limit)
	new_value = min_width_limit;
      else if (new_value > width_limit)
	new_value = width_limit;
      stp_set_page_width (pv->v, new_value);
    }
  else
    {
      if (new_value < min_height_limit)
	new_value = min_height_limit;
      else if (new_value > height_limit)
	new_value = height_limit;
      stp_set_page_height (pv->v, new_value);
    }
  set_entry_value (widget, new_value, 0);
  preview_update ();
}


/*
 *  media_size_callback() - Update the current media size.
 */
static void
set_media_size(const gchar *new_media_size)
{
  const stp_papersize_t pap = stp_get_papersize_by_name (new_media_size);

  if (pap)
    {
      gint default_width, default_height;
      gint size;

      if (stp_papersize_get_width (pap) == 0)
	{
	  stp_printer_get_media_size
	    (current_printer, pv->v, &default_width, &default_height);
	  gtk_widget_set_sensitive (GTK_WIDGET (custom_size_width), TRUE);
	  gtk_entry_set_editable (GTK_ENTRY (custom_size_width), TRUE);
	  size = default_width;
	}
      else
	{
	  size = stp_papersize_get_width (pap);
	  gtk_widget_set_sensitive (GTK_WIDGET (custom_size_width), FALSE);
	  gtk_entry_set_editable (GTK_ENTRY (custom_size_width), FALSE);
	}
      set_entry_value (custom_size_width, size, 0);
      stp_set_page_width (pv->v, size);

      if (stp_papersize_get_height (pap) == 0)
	{
	  stp_printer_get_media_size
	    (current_printer, pv->v, &default_height, &default_height);
	  gtk_widget_set_sensitive (GTK_WIDGET (custom_size_height), TRUE);
	  gtk_entry_set_editable (GTK_ENTRY (custom_size_height), TRUE);
	  size = default_height;
	}
      else
	{
	  size = stp_papersize_get_height (pap);
	  gtk_widget_set_sensitive(GTK_WIDGET (custom_size_height), FALSE);
	  gtk_entry_set_editable (GTK_ENTRY (custom_size_height), FALSE);
	}
      set_entry_value (custom_size_height, size, 0);
      stp_set_page_height (pv->v, size);
    }
}

static void
combo_callback(GtkWidget *widget, gpointer data)
{
  list_option_t *option = (list_option_t *)data;
  const gchar *new_value = Combo_get_name(option->combo, option->count,
					  option->params);
  reset_preview();
  if (strcmp(stp_get_parameter(pv->v, option->name), new_value) != 0)
    {
      invalidate_frame();
      invalidate_preview_thumbnail();
      if (option->extra)
	(option->extra)(new_value);
      stp_set_parameter(pv->v, option->name, new_value);
      preview_update();
    }
}

/*
 *  orientation_callback() - Update the current media size.
 */
static void
orientation_callback (GtkWidget *widget,
		      gpointer   data)
{
  reset_preview ();

  if (pv->orientation != (gint) data)
    {
      invalidate_preview_thumbnail ();
      set_orientation((gint) data);
      preview_update ();
    }
}

/*
 *  output_type_callback() - Update the current output type.
 */
static void
output_type_callback (GtkWidget *widget,
		      gpointer   data)
{
  reset_preview ();

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      stp_set_output_type (pv->v, (gint) data);
      invalidate_preview_thumbnail ();
      update_adjusted_thumbnail ();
      if (widget == output_types[0].button)
	set_color_sliders_active (TRUE);
      else
	set_color_sliders_active (FALSE);
      preview_update ();
    }
}

static void
set_all_entry_values(void)
{
  set_entry_value (top_entry, (stp_get_top (pv->v)), 1);
  set_entry_value (left_entry, (stp_get_left (pv->v)), 1);
  set_entry_value (bottom_entry, (top + stp_get_top(pv->v) + print_height), 1);
  set_entry_value (bottom_border_entry,
                   (paper_height - (stp_get_top (pv->v) + print_height)), 1);
  set_entry_value (right_entry, (stp_get_left(pv->v) + print_width), 1);
  set_entry_value (right_border_entry,
                   (paper_width - (stp_get_left (pv->v) + print_width)), 1);
  set_entry_value (width_entry, print_width, 1);
  set_entry_value (height_entry, print_height, 1);
  set_entry_value (custom_size_width, stp_get_page_width (pv->v), 1);
  set_entry_value (custom_size_height, stp_get_page_height (pv->v), 1);
}

/*
 *  unit_callback() - Update the current unit.
 */
static void
unit_callback (GtkWidget *widget,
	       gpointer   data)
{
  reset_preview ();

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      pv->unit = (gint) data;
      set_all_entry_values();
    }
}

/*
 *  image_type_callback() - Update the current image type mode.
 */
static void
image_type_callback (GtkWidget *widget,
		     gpointer   data)
{
  reset_preview ();

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      stp_set_image_type (pv->v, (gint) data);
      invalidate_preview_thumbnail ();
      update_adjusted_thumbnail ();
      preview_update ();
    }
}

static void
destroy_dialogs (void)
{
  gtk_widget_destroy (color_adjust_dialog);
  gtk_widget_destroy (setup_dialog);
  gtk_widget_destroy (print_dialog);
  gtk_widget_destroy (new_printer_dialog);
  gtk_widget_destroy (about_dialog);
}

static void
dialogs_set_sensitive (gboolean sensitive)
{
  gtk_widget_set_sensitive (color_adjust_dialog, sensitive);
  gtk_widget_set_sensitive (setup_dialog, sensitive);
  gtk_widget_set_sensitive (print_dialog, sensitive);
  gtk_widget_set_sensitive (new_printer_dialog, sensitive);
  gtk_widget_set_sensitive (about_dialog, sensitive);
}

/*
 * 'print_callback()' - Start the print.
 */
static void
print_callback (void)
{
  if (plist_current > 0)
    {
      runme = TRUE;
      destroy_dialogs ();
    }
  else
    {
      dialogs_set_sensitive (FALSE);
      gtk_widget_show (file_browser);
    }
}

/*
 *  printandsave_callback() -
 */
static void
printandsave_callback (void)
{
  saveme = TRUE;
  print_callback();
}

static void
about_callback (void)
{
  gtk_widget_show (about_dialog);
}

/*
 *  save_callback() - save settings, don't destroy dialog
 */
static void
save_callback (void)
{
  reset_preview ();
  printrc_save ();
}

/*
 *  setup_update() - update widgets in the setup dialog
 */
static void
setup_update (void)
{
  GtkAdjustment *adjustment;
  gint           idx;

  current_printer = stp_get_printer_by_driver (stp_get_driver (pv->v));
  idx = stp_get_printer_index_by_driver (stp_get_driver (pv->v));

  gtk_clist_select_row (GTK_CLIST (printer_driver), idx, 0);

  gtk_entry_set_text (GTK_ENTRY (ppd_file), stp_get_ppd_file (pv->v));

  if (strncmp (stp_get_driver (pv->v),"ps", 2) == 0)
    {
      gtk_widget_show (ppd_label);
      gtk_widget_show (ppd_file);
      gtk_widget_show (ppd_button);
    }
  else
    {
      gtk_widget_hide (ppd_label);
      gtk_widget_hide (ppd_file);
      gtk_widget_hide (ppd_button);
    }

  gtk_entry_set_text (GTK_ENTRY (output_cmd), plist_get_output_to (pv));

  if (plist_current == 0)
    gtk_widget_hide (output_cmd);
  else
    gtk_widget_show (output_cmd);

  adjustment = GTK_CLIST (printer_driver)->vadjustment;
  gtk_adjustment_set_value (adjustment,
                            adjustment->lower +
                            idx * (adjustment->upper - adjustment->lower) /
                            GTK_CLIST (printer_driver)->rows);
}

/*
 *  setup_open_callback() -
 */
static void
setup_open_callback (void)
{
  static gboolean first_time = TRUE;

  reset_preview ();
  setup_update ();

  gtk_widget_show (setup_dialog);

  if (first_time)
    {
      /* Make sure the driver scroller gets positioned correctly. */
      setup_update ();
      first_time = FALSE;
    }
}

/*
 *  new_printer_open_callback() -
 */
static void
new_printer_open_callback (void)
{
  reset_preview ();
  gtk_entry_set_text (GTK_ENTRY (new_printer_entry), "");
  gtk_widget_show (new_printer_dialog);
}

static void
set_printer(void)
{
  stp_set_driver (pv->v, stp_printer_get_driver (current_printer));
  plist_set_output_to (pv, gtk_entry_get_text (GTK_ENTRY (output_cmd)));
  stp_set_ppd_file (pv->v, gtk_entry_get_text (GTK_ENTRY (ppd_file)));
  plist_callback (NULL, (gpointer) plist_current);
}

/*
 *  setup_ok_callback() -
 */
static void
setup_ok_callback (void)
{
  set_printer();
  gtk_widget_hide (setup_dialog);
}

/*
 *  setup_ok_callback() -
 */
static void
new_printer_ok_callback (void)
{
  const gchar *data = gtk_entry_get_text (GTK_ENTRY (new_printer_entry));
  gp_plist_t   key;

  if (strlen(data))
    {
      memset(&key, 0, sizeof(key));
      initialize_printer (&key);
      copy_printer(&key, pv);
      plist_set_name(&key, data);

      key.active = 0;

      if (add_printer (&key, 1))
	{
	  plist_current = plist_count - 1;
	  build_printer_combo ();
	  set_printer();
	}
    }

  gtk_widget_hide (new_printer_dialog);
}

/*
 *  print_driver_callback() - Update the current printer driver.
 */
static void
print_driver_callback (GtkWidget      *widget, /* I - Driver list */
		       gint            row,
		       gint            column,
		       GdkEventButton *event,
		       gpointer        data)   /* I - Data */
{
  stp_vars_t printvars;

  invalidate_frame ();
  invalidate_preview_thumbnail ();
  reset_preview ();
  data = gtk_clist_get_row_data (GTK_CLIST (widget), row);
  current_printer = stp_get_printer_by_index ((gint) data);
  gtk_label_set_text (GTK_LABEL (printer_model_label),
                      gettext (stp_printer_get_long_name (current_printer)));

  if (strncmp (stp_printer_get_driver (current_printer), "ps", 2) == 0)
    {
      gtk_widget_show (ppd_label);
      gtk_widget_show (ppd_file);
      gtk_widget_show (ppd_button);
    }
  else
    {
      gtk_widget_hide (ppd_label);
      gtk_widget_hide (ppd_file);
      gtk_widget_hide (ppd_button);
    }

  printvars = stp_printer_get_printvars (current_printer);

  if (stp_get_output_type (printvars) == OUTPUT_COLOR)
    {
      gtk_widget_set_sensitive (output_types[0].button, TRUE);
    }
  else
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (output_types[1].button),
				    TRUE);
      gtk_widget_set_sensitive (output_types[0].button, FALSE);
    }
}

/*
 *  ppd_browse_callback() -
 */
static void
ppd_browse_callback (void)
{
  reset_preview ();
  gtk_file_selection_set_filename (GTK_FILE_SELECTION (ppd_browser),
				   gtk_entry_get_text (GTK_ENTRY (ppd_file)));
  gtk_widget_show (ppd_browser);
}

/*
 *  ppd_ok_callback() -
 */
static void
ppd_ok_callback (void)
{
  reset_preview ();
  gtk_widget_hide (ppd_browser);
  gtk_entry_set_text
    (GTK_ENTRY (ppd_file),
     gtk_file_selection_get_filename (GTK_FILE_SELECTION (ppd_browser)));
}

/*
 *  file_ok_callback() - print to file and go away
 */
static void
file_ok_callback (void)
{
  gtk_widget_hide (file_browser);
  plist_set_output_to (pv,
		       gtk_file_selection_get_filename (GTK_FILE_SELECTION (file_browser)));

  runme = TRUE;
  destroy_dialogs ();
}

/*
 *  file_cancel_callback() -
 */
static void
file_cancel_callback (void)
{
  gtk_widget_hide (file_browser);
  dialogs_set_sensitive (TRUE);
}

/*
 * update_adjusted_thumbnail()
 */
void
update_adjusted_thumbnail (void)
{
  gint           x, y;
  stp_convert_t  colorfunc;
  gushort        out[3 * (THUMBNAIL_MAXW + THUMBNAIL_MAXH)];
  guchar        *adjusted_data = adjusted_thumbnail_data;
  gfloat         old_density = stp_get_density(pv->v);
  gint preview_limit = (thumbnail_h * thumbnail_w) - 1;

  if (thumbnail_data == 0 || adjusted_thumbnail_data == 0)
    return;

  stp_set_density (pv->v, 1.0);
  stp_compute_lut (pv->v, 256);
  colorfunc = stp_choose_colorfunc (stp_get_output_type(pv->v), thumbnail_bpp,
				    NULL, &adjusted_thumbnail_bpp, pv->v);
  for (y = 0; y < thumbnail_h; y++)
    {
      (*colorfunc) (pv->v, thumbnail_data + thumbnail_bpp * thumbnail_w * y,
		    out, NULL, thumbnail_w, thumbnail_bpp, NULL, NULL, NULL,
		    NULL);
      for (x = 0; x < adjusted_thumbnail_bpp * thumbnail_w; x++)
	{
	  *adjusted_data++ = out[x] / 0x0101U;
	}
    }

  stp_free_lut (pv->v);

  stp_set_density (pv->v, old_density);

  switch (physical_orientation)
    {
    case ORIENT_PORTRAIT:
      memcpy(preview_thumbnail_data, adjusted_thumbnail_data,
	     adjusted_thumbnail_bpp * thumbnail_h * thumbnail_w);
      break;
    case ORIENT_SEASCAPE:
      for (x = 0; x < thumbnail_w; x++)
	for (y = 0; y < thumbnail_h; y++)
	  memcpy((preview_thumbnail_data +
		  adjusted_thumbnail_bpp * (x * thumbnail_h + y)),
		 (adjusted_thumbnail_data +
		  adjusted_thumbnail_bpp * (y * thumbnail_w + x)),
		 adjusted_thumbnail_bpp);
      break;

    case ORIENT_UPSIDEDOWN:
      for (x = 0; x < thumbnail_h * thumbnail_w; x++)
	memcpy((preview_thumbnail_data +
		adjusted_thumbnail_bpp * (preview_limit - x)),
	       adjusted_thumbnail_data + adjusted_thumbnail_bpp * x,
	       adjusted_thumbnail_bpp);
      break;
    case ORIENT_LANDSCAPE:
      for (x = 0; x < thumbnail_w; x++)
	for (y = 0; y < thumbnail_h; y++)
	  memcpy((preview_thumbnail_data +
		  adjusted_thumbnail_bpp * (preview_limit -
					    (x * thumbnail_h + y))),
		 (adjusted_thumbnail_data +
		  adjusted_thumbnail_bpp * (y * thumbnail_w + x)),
		 adjusted_thumbnail_bpp);
      break;
    }

  redraw_color_swatch ();
  preview_update ();
}

void
invalidate_preview_thumbnail (void)
{
  preview_valid = 0;
}

void
invalidate_frame (void)
{
  frame_valid = 0;
}

static void
draw_arrow (GdkWindow *w,
            GdkGC     *gc,
            gint       paper_left,
            gint       paper_top)
{
  gint u  = preview_ppi/2;
  gint ox = paper_left + preview_ppi * paper_width / INCH / 2;
  gint oy = paper_top + preview_ppi * paper_height / INCH / 2;

  oy -= preview_ppi * paper_height / INCH / 4;
  if (oy < paper_top + u)
    oy = paper_top + u;
  gdk_draw_line (w, gc, ox, oy - u, ox - u, oy);
  gdk_draw_line (w, gc, ox, oy - u, ox + u, oy);
  gdk_draw_line (w, gc, ox, oy - u, ox, oy + u);
}

/*
 *  preview_update_callback() -
 */
static void
do_preview_thumbnail (void)
{
  static GdkGC	*gc    = NULL;
  static GdkGC  *gcinv = NULL;
  static GdkGC  *gcset = NULL;
  static guchar *preview_data = NULL;
  static gint    opx = 0;
  static gint    opy = 0;
  static gint    oph = 0;
  static gint    opw = 0;
  gint paper_display_left, paper_display_top;
  gint printable_display_left, printable_display_top;
  gint preview_x, preview_y, preview_r, preview_b;
  gint preview_w, preview_h;
  gint paper_display_width, paper_display_height;
  gint printable_display_width, printable_display_height;
  int bottom = stp_get_top(pv->v) + stp_get_height(pv->v);
  int right = stp_get_left(pv->v) + stp_get_width(pv->v);

  preview_ppi = PREVIEW_SIZE_HORIZ * FINCH / (gdouble) paper_width;

  if (preview_ppi > PREVIEW_SIZE_VERT * FINCH / (gdouble) paper_height)
    preview_ppi = PREVIEW_SIZE_VERT * FINCH / (gdouble) paper_height;
  if (preview_ppi > MAX_PREVIEW_PPI)
    preview_ppi = MAX_PREVIEW_PPI;

  if (preview == NULL || preview->widget.window == NULL)
    return;
  /*
   * Center the page on the preview
   */
  paper_display_width = MAX(3, ROUNDUP(preview_ppi * paper_width, INCH));
  paper_display_height = MAX(3, ROUNDUP(preview_ppi * paper_height, INCH));

  paper_display_left = (PREVIEW_SIZE_HORIZ - paper_display_width) / 2;
  paper_display_top = (PREVIEW_SIZE_VERT - paper_display_height) / 2;

  printable_display_width =
    MAX(3, ROUNDUP(preview_ppi * printable_width, INCH));
  printable_display_height =
    MAX(3, ROUNDUP(preview_ppi * printable_height, INCH));

  printable_display_left = paper_display_left + preview_ppi * left / INCH;
  printable_display_top  = paper_display_top + preview_ppi * top / INCH ;

  preview_x =
    1 + paper_display_left + preview_ppi * stp_get_left (pv->v) / INCH;
  preview_y =
    1 + paper_display_top + preview_ppi * stp_get_top (pv->v) / INCH;

  preview_r = 1 + paper_display_left + preview_ppi * right / INCH;
  preview_b = 1 + paper_display_top + preview_ppi * bottom / INCH;
  preview_w = preview_r - preview_x;
  preview_h = preview_b - preview_y;


  if (preview_w + preview_x > printable_display_left + printable_display_width)
    preview_w--;
  if (preview_h + preview_y > printable_display_top + printable_display_height)
    preview_h--;

  if (gc == NULL)
    {
      gc = gdk_gc_new (preview->widget.window);
      gcinv = gdk_gc_new (preview->widget.window);
      gdk_gc_set_function (gcinv, GDK_INVERT);
      gcset = gdk_gc_new (preview->widget.window);
      gdk_gc_set_function (gcset, GDK_SET);
    }

  if (!preview_valid)
    {
      gint v_denominator = preview_h > 1 ? preview_h - 1 : 1;
      gint v_numerator = (preview_thumbnail_h - 1) % v_denominator;
      gint v_whole = (preview_thumbnail_h - 1) / v_denominator;
      gint h_denominator = preview_w > 1 ? preview_w - 1 : 1;
      gint h_numerator = (preview_thumbnail_w - 1) % h_denominator;
      gint h_whole = (preview_thumbnail_w - 1) / h_denominator;
      gint adjusted_preview_width = adjusted_thumbnail_bpp * preview_w;
      gint adjusted_thumbnail_width = adjusted_thumbnail_bpp *
	preview_thumbnail_w;
      gint v_cur = 0;
      gint v_last = -1;
      gint v_error = v_denominator / 2;
      gint y;
      gint i;

      if (preview_data)
	free (preview_data);
      preview_data = g_malloc (3 * preview_h * preview_w);

      for (y = 0; y < preview_h; y++)
	{
	  guchar *outbuf = preview_data + adjusted_preview_width * y;

	  if (v_cur == v_last)
	    {
	      memcpy (outbuf, outbuf - adjusted_preview_width,
                      adjusted_preview_width);
	    }
	  else
	    {
	      guchar *inbuf = preview_thumbnail_data - adjusted_thumbnail_bpp
		+ adjusted_thumbnail_width * v_cur;

	      gint h_cur = 0;
	      gint h_last = -1;
	      gint h_error = h_denominator / 2;
	      gint x;

	      v_last = v_cur;
	      for (x = 0; x < preview_w; x++)
		{
		  if (h_cur == h_last)
		    {
		      for (i = 0; i < adjusted_thumbnail_bpp; i++)
			outbuf[i] = outbuf[i - adjusted_thumbnail_bpp];
		    }
		  else
		    {
		      inbuf += adjusted_thumbnail_bpp * (h_cur - h_last);
		      h_last = h_cur;
		      for (i = 0; i < adjusted_thumbnail_bpp; i++)
			outbuf[i] = inbuf[i];
		    }
		  outbuf += adjusted_thumbnail_bpp;
		  h_cur += h_whole;
		  h_error += h_numerator;
		  if (h_error >= h_denominator)
		    {
		      h_error -= h_denominator;
		      h_cur++;
		    }
		}
	    }
	  v_cur += v_whole;
	  v_error += v_numerator;
	  if (v_error >= v_denominator)
	    {
	      v_error -= v_denominator;
	      v_cur++;
	    }
	}
      preview_valid = 1;
    }

  if (need_exposure)
    {
      /* draw paper frame */
      gdk_draw_rectangle (preview->widget.window, gc, 0,
			  paper_display_left, paper_display_top,
			  paper_display_width, paper_display_height);

      /* draw printable frame */
      gdk_draw_rectangle (preview->widget.window, gc, 0,
			  printable_display_left, printable_display_top,
			  printable_display_width, printable_display_height);
      need_exposure = 0;
    }
  else if (!frame_valid)
    {
      gdk_window_clear (preview->widget.window);
      /* draw paper frame */
      gdk_draw_rectangle (preview->widget.window, gc, 0,
			  paper_display_left, paper_display_top,
			  paper_display_width, paper_display_height);

      /* draw printable frame */
      gdk_draw_rectangle (preview->widget.window, gc, 0,
			  printable_display_left, printable_display_top,
			  printable_display_width, printable_display_height);
      frame_valid = 1;
    }
  else
    {
      if (opx + opw <= preview_x || opy + oph <= preview_y ||
	  preview_x + preview_w <= opx || preview_y + preview_h <= opy)
        {
          gdk_window_clear_area (preview->widget.window, opx, opy, opw, oph);
        }
      else
	{
	  if (opx < preview_x)
	    gdk_window_clear_area (preview->widget.window,
                                   opx, opy, preview_x - opx, oph);
	  if (opy < preview_y)
	    gdk_window_clear_area (preview->widget.window,
                                   opx, opy, opw, preview_y - opy);
	  if (opx + opw > preview_x + preview_w)
	    gdk_window_clear_area (preview->widget.window,
                                   preview_x + preview_w, opy,
                                   (opx + opw) - (preview_x + preview_w), oph);
	  if (opy + oph > preview_y + preview_h)
	    gdk_window_clear_area (preview->widget.window,
                                   opx, preview_y + preview_h,
                                   opw, (opy + oph) - (preview_y + preview_h));
	}
    }

  draw_arrow (preview->widget.window, gcset, paper_display_left,
	      paper_display_top);

  if (adjusted_thumbnail_bpp == 1)
    gdk_draw_gray_image (preview->widget.window, gc,
			 preview_x, preview_y, preview_w, preview_h,
			 GDK_RGB_DITHER_NORMAL, preview_data, preview_w);
  else
    gdk_draw_rgb_image (preview->widget.window, gc,
			preview_x, preview_y, preview_w, preview_h,
			GDK_RGB_DITHER_NORMAL, preview_data, 3 * preview_w);

  /* draw orientation arrow pointing to top-of-paper */
  draw_arrow (preview->widget.window, gcinv, paper_display_left,
	      paper_display_top);
  gdk_flush();

  opx = preview_x;
  opy = preview_y;
  oph = preview_h;
  opw = preview_w;
}

static void
preview_expose (void)
{
  need_exposure = 1;
  preview_update ();
}

static void
preview_update (void)
{
  gdouble max_ppi_scaling;   /* Maximum PPI for current page size */
  gdouble min_ppi_scaling;   /* Minimum PPI for current page size */

  suppress_preview_update++;
  stp_printer_get_media_size(current_printer, pv->v,
			     &paper_width, &paper_height);

  stp_printer_get_imageable_area(current_printer, pv->v,
				 &left, &right, &bottom, &top);

  printable_width  = right - left;
  printable_height = bottom - top;

  if (pv->scaling < 0)
    {
      gdouble twidth;

      compute_scaling_limits(&min_ppi_scaling, &max_ppi_scaling);

      if (pv->scaling < 0 && pv->scaling > -min_ppi_scaling)
	pv->scaling = -min_ppi_scaling;

      twidth = (FINCH * (gdouble) image_width / -pv->scaling);
      print_width = twidth + .5;
      print_height = (twidth * (gdouble) image_height / image_width) + .5;
      GTK_ADJUSTMENT (scaling_adjustment)->lower = min_ppi_scaling;
      GTK_ADJUSTMENT (scaling_adjustment)->upper = max_ppi_scaling;
      GTK_ADJUSTMENT (scaling_adjustment)->value = -pv->scaling;

      if (!suppress_scaling_adjustment)
	{
	  suppress_preview_reset++;
	  gtk_adjustment_changed (GTK_ADJUSTMENT (scaling_adjustment));
	  suppress_scaling_callback = TRUE;
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scaling_ppi), TRUE);
	  suppress_scaling_callback = FALSE;
	  gtk_adjustment_value_changed (GTK_ADJUSTMENT (scaling_adjustment));
	  suppress_preview_reset--;
	}
    }
  else
    {
      /* we do pv->scaling % of height or width, whatever is less */
      /* this is relative to printable size */
      if (image_width * printable_height > printable_width * image_height)
	/* if image_width/image_height > printable_width/printable_height */
	/* i.e. if image is wider relative to its height than the width
	   of the printable area relative to its height */
	{
	  gdouble twidth = .5 + printable_width * pv->scaling / 100;

	  print_width = twidth;
	  print_height = twidth * (gdouble) image_height /
	    (gdouble) image_width;
	}
      else
	{
	  gdouble theight = .5 + printable_height * pv->scaling /100;

	  print_height = theight;
	  print_width = theight * (gdouble) image_width /
	    (gdouble) image_height;
	}
    }
  stp_set_width(pv->v, print_width);
  stp_set_height(pv->v, print_height);


  if (!pv->left_is_valid)
    {
      stp_set_left (pv->v, (paper_width - print_width) / 2);
      pv->left_is_valid = 1;
    }
  if (stp_get_left(pv->v) < left)
    stp_set_left(pv->v, left);

  /* we leave stp_get_left(pv->v) etc. relative to printable area */
  if (stp_get_left (pv->v) > right - print_width)
    stp_set_left (pv->v, right - print_width);

  if (!pv->top_is_valid)
    {
      stp_set_top (pv->v, ((paper_height - print_height) / 2));
      pv->top_is_valid = 1;
    }
  if (stp_get_top(pv->v) < top)
    stp_set_top(pv->v, top);

  if (stp_get_top (pv->v) > bottom - print_height)
    stp_set_top (pv->v, bottom - print_height);

  set_all_entry_values();
  suppress_preview_update--;

  /* draw image */
  if (! suppress_preview_update)
    do_preview_thumbnail ();
}

/*
 *  preview_button_callback() -
 */
static void
preview_button_callback (GtkWidget      *widget,
			 GdkEventButton *event,
			 gpointer        data)
{
  if (event->type == GDK_BUTTON_PRESS)
    {
      if (preview_active == 0)
	{
	  mouse_x = event->x;
	  mouse_y = event->y;
	  orig_left = stp_get_left (pv->v);
	  orig_top = stp_get_top (pv->v);
	  mouse_button = event->button;
	  buttons_mask = 1 << event->button;
	  buttons_pressed++;
	  preview_active = 1;
	  disable_help();
	  if (event->state & GDK_SHIFT_MASK)
	    move_constraint = MOVE_CONSTRAIN;
	  else
	    move_constraint = MOVE_ANY;
	}
      else if (preview_active == 1)
	{
	  if ((buttons_mask & (1 << event->button)) == 0)
	    {
	      enable_help();
	      preview_active = -1;
	      stp_set_left (pv->v, orig_left);
	      stp_set_top (pv->v, orig_top);
	      preview_update ();
	      buttons_mask |= 1 << event->button;
	      buttons_pressed++;
	    }
	}
      else
	{
	  if ((buttons_mask & (1 << event->button)) == 0)
	    {
	      buttons_mask |= 1 << event->button;
	      buttons_pressed++;
	    }
	}
    }
  else if (event->type == GDK_BUTTON_RELEASE)
    {
      buttons_pressed--;
      buttons_mask &= ~(1 << event->button);
      if (buttons_pressed == 0)
	{
	  enable_help ();
	  preview_active = 0;
	}
    }
}

/*
 *  preview_motion_callback() -
 */
static void
preview_motion_callback (GtkWidget      *widget,
			 GdkEventMotion *event,
			 gpointer        data)
{
  if (event->type != GDK_MOTION_NOTIFY)
    return;
  if (preview_active != 1)
    return;
  if (move_constraint == MOVE_CONSTRAIN)
    {
      int dx = abs(event->x - mouse_x);
      int dy = abs(event->y - mouse_y);
      if (dx > dy && dx > 3)
	move_constraint = MOVE_HORIZONTAL;
      else if (dy > dx && dy > 3)
	move_constraint = MOVE_VERTICAL;
      else
	return;
    }

  if (mouse_button == 2)
    {
      int changes = 0;
      int y_threshold = MAX (1, (preview_ppi * print_height) / INCH);

      if (move_constraint & MOVE_HORIZONTAL)
	{
	  int x_threshold = MAX (1, (preview_ppi * print_width) / INCH);
	  while (event->x - mouse_x >= x_threshold)
	    {
	      if (stp_get_left (pv->v) + (print_width * 2) <= right)
		{
		  stp_set_left (pv->v, stp_get_left (pv->v) + print_width);
		  mouse_x += x_threshold;
		  changes = 1;
		}
	      else
		break;
	    }
	  while (mouse_x - event->x >= x_threshold)
	    {
	      if (stp_get_left (pv->v) >= print_width + left)
		{
		  stp_set_left (pv->v, stp_get_left (pv->v) - print_width);
		  mouse_x -= x_threshold;
		  changes = 1;
		}
	      else
		break;
	    }
	}

      if (move_constraint & MOVE_VERTICAL)
	{
	  while (event->y - mouse_y >= y_threshold)
	    {
	      if (stp_get_top (pv->v) + (print_height * 2) <= bottom)
		{
		  stp_set_top (pv->v, stp_get_top (pv->v) + print_height);
		  mouse_y += y_threshold;
		  changes = 1;
		}
	      else
		break;
	    }
	  while (mouse_y - event->y >= y_threshold)
	    {
	      if (stp_get_top (pv->v) >= print_height + top)
		{
		  stp_set_top (pv->v, stp_get_top (pv->v) - print_height);
		  mouse_y -= y_threshold;
		  changes = 1;
		}
	      else
		break;
	    }
	}
      if (!changes)
	return;
    }
  else
    {
      gint old_top  = stp_get_top (pv->v);
      gint old_left = stp_get_left (pv->v);
      gint new_top  = old_top;
      gint new_left = old_left;
      gint changes  = 0;

      if (mouse_button == 1)
	{
	  if (move_constraint & MOVE_VERTICAL)
	    new_top = orig_top + INCH * (event->y - mouse_y) / preview_ppi;
	  if (move_constraint & MOVE_HORIZONTAL)
	    new_left = orig_left + INCH * (event->x - mouse_x) / preview_ppi;
	}
      else
	{
	  if (move_constraint & MOVE_VERTICAL)
	    new_top = orig_top + event->y - mouse_y;
	  if (move_constraint & MOVE_HORIZONTAL)
	    new_left = orig_left + event->x - mouse_x;
	}

      if (new_top < top)
	new_top = top;
      if (new_top > bottom - print_height)
	new_top = bottom - print_height;
      if (new_left < left)
	new_left = left;
      if (new_left > right - print_width)
	new_left = right - print_width;

      if (new_top != old_top)
	{
	  stp_set_top (pv->v, new_top);
	  changes = 1;
	}
      if (new_left != old_left)
	{
	  stp_set_left (pv->v, new_left);
	  changes = 1;
	}
      if (!changes)
	return;
    }

  preview_update ();
}
