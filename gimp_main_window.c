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

#include "print_gimp.h"

#ifndef GIMP_1_0

#include "print-intl.h"

/*
 * Constants for GUI...
 */

extern vars_t           vars;
extern gint             plist_count;	   /* Number of system printers */
extern gint             plist_current;     /* Current system printer */
extern plist_t          *plist;		  /* System printers */
extern gint32           image_ID;
extern gint             image_width;
extern gint             image_height;
extern const printer_t *current_printer;
extern gint             runme;
extern gint             saveme;

extern GtkWidget *gimp_color_adjust_dialog;

void  printrc_save (void);

/*
 *  Main window widgets
 */
static GtkWidget *print_dialog;           /* Print dialog window */
static GtkWidget *recenter_button;
static GtkWidget *left_entry;
static GtkWidget *right_entry;
static GtkWidget *width_entry;
static GtkWidget *top_entry;
static GtkWidget *bottom_entry;
static GtkWidget *height_entry;
static GtkWidget *media_size;             /* Media size option button */
static GtkWidget *media_size_menu=NULL;   /* Media size menu */
static GtkWidget *media_type;             /* Media type option button */
static GtkWidget *media_type_menu=NULL;   /* Media type menu */
static GtkWidget *media_source;           /* Media source option button */
static GtkWidget *media_source_menu=NULL; /* Media source menu */
static GtkWidget *ink_type;               /* Ink type option button */
static GtkWidget *ink_type_menu=NULL;     /* Ink type menu */
static GtkWidget *resolution;             /* Resolution option button */
static GtkWidget *resolution_menu=NULL;   /* Resolution menu */
static GtkWidget *scaling_percent;        /* Scale by percent */
static GtkWidget *scaling_ppi;            /* Scale by pixels-per-inch */
static GtkWidget *scaling_image;          /* Scale to the image */
static GtkWidget *output_gray;            /* Output type toggle, black */
static GtkWidget *output_color;           /* Output type toggle, color */
static GtkWidget *image_line_art;
static GtkWidget *image_solid_tone;
static GtkWidget *image_continuous_tone;
static GtkWidget *image_monochrome;
static GtkWidget *setup_dialog;         /* Setup dialog window */
static GtkWidget *printer_driver;       /* Printer driver widget */
static GtkWidget *ppd_file;             /* PPD file entry */
static GtkWidget *ppd_button;           /* PPD file browse button */
static GtkWidget *output_cmd;           /* Output command text entry */
static GtkWidget *ppd_browser;          /* File selection dialog for PPD files */
static GtkWidget *file_browser;         /* FSD for print files */
static GtkWidget *printandsave_button;
static GtkWidget *adjust_color_button;

static GtkObject *scaling_adjustment;	/* Adjustment object for scaling */

static gint    num_media_sizes = 0;	/* Number of media sizes */
static gchar **media_sizes;		/* Media size strings */
static gint    num_media_types = 0;	/* Number of media types */
static gchar **media_types;		/* Media type strings */
static gint    num_media_sources = 0;	/* Number of media sources */
static gchar **media_sources;	        /* Media source strings */
static gint    num_ink_types = 0;	/* Number of ink types */
static gchar **ink_types;		/* Ink type strings */
static gint    num_resolutions = 0;	/* Number of resolutions */
static gchar **resolutions;		/* Resolution strings */


static GtkDrawingArea *preview;		/* Preview drawing area widget */
static gint            mouse_x;		/* Last mouse X */
static gint            mouse_y;		/* Last mouse Y */

static gint            printable_left;	/* Left pixel column of page */
static gint            printable_top;	/* Top pixel row of page */
static gint            printable_width;	/* Width of page on screen */
static gint            printable_height;	/* Height of page on screen */
static gint            print_width;	/* Printed width of image */
static gint            print_height;	/* Printed height of image */
static gint	       left, right;	        /* Imageable area */
static gint            top, bottom;
static gint	       paper_width, paper_height;	/* Physical width */

extern GtkObject *brightness_adjustment; /* Adjustment object for brightness */
extern GtkObject *saturation_adjustment; /* Adjustment object for saturation */
extern GtkObject *density_adjustment;	 /* Adjustment object for density */
extern GtkObject *contrast_adjustment;	 /* Adjustment object for contrast */
extern GtkObject *red_adjustment;	 /* Adjustment object for red */
extern GtkObject *green_adjustment;	 /* Adjustment object for green */
extern GtkObject *blue_adjustment;	 /* Adjustment object for blue */
extern GtkObject *gamma_adjustment;	 /* Adjustment object for gamma */

static void gimp_scaling_update        (GtkAdjustment *adjustment);
static void gimp_scaling_callback      (GtkWidget     *widget);
static void gimp_plist_callback        (GtkWidget     *widget,
					gpointer       data);
static void gimp_media_size_callback   (GtkWidget     *widget,
					gpointer       data);
static void gimp_media_type_callback   (GtkWidget     *widget,
					gpointer       data);
static void gimp_media_source_callback (GtkWidget     *widget,
					gpointer       data);
static void gimp_ink_type_callback     (GtkWidget     *widget,
					gpointer       data);
static void gimp_resolution_callback   (GtkWidget     *widget,
					gpointer       data);
static void gimp_output_type_callback  (GtkWidget     *widget,
					gpointer       data);
#ifdef DO_LINEAR
static void gimp_linear_callback       (GtkWidget     *widget,
					gpointer       data);
#endif
static void gimp_orientation_callback  (GtkWidget     *widget,
					gpointer       data);
static void gimp_printandsave_callback (void);
static void gimp_print_callback        (void);
static void gimp_save_callback         (void);

static void gimp_setup_open_callback   (void);
static void gimp_setup_ok_callback     (void);
static void gimp_ppd_browse_callback   (void);
static void gimp_ppd_ok_callback       (void);
static void gimp_print_driver_callback (GtkWidget     *widget,
					gpointer       data);

static void gimp_file_ok_callback      (void);
static void gimp_file_cancel_callback  (void);

static void gimp_preview_update              (void);
static void gimp_preview_button_callback     (GtkWidget      *widget,
					      GdkEventButton *bevent,
					      gpointer        data);
static void gimp_preview_motion_callback     (GtkWidget      *widget,
					      GdkEventMotion *mevent,
					      gpointer        data);
static void gimp_position_callback           (GtkWidget      *widget);
static void gimp_image_type_callback         (GtkWidget      *widget,
					      gpointer        data);

extern void gimp_create_color_adjust_window  (void);

/*
 *  gimp_create_main_window()
 */
void
gimp_create_main_window (void)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *printer_table;

  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *entry;
  GtkWidget *menu;
  GtkWidget *item;
  GtkWidget *option;
  GtkWidget *box;
  GSList    *group;
#ifdef DO_LINEAR
  GSList    *linear_group;
#endif
  GSList    *image_type_group;
  gint       i;
  gchar      s[100];

  const printer_t *the_printer = get_printer_by_index (0);
  gchar *plug_in_name;

  /*
   * Create the main dialog
   */

  plug_in_name = g_strdup_printf (_("Print v%s"), PLUG_IN_VERSION);

  print_dialog = dialog =
    gimp_dialog_new (plug_in_name, "print",
                     gimp_plugin_help_func, "filters/print.html",
                     GTK_WIN_POS_MOUSE,
                     FALSE, TRUE, FALSE,

                     _("Print and\nSave Settings"), gimp_printandsave_callback,
                     NULL, NULL, &printandsave_button, FALSE, FALSE,
                     _("Save\nSettings"), gimp_save_callback,
                     NULL, NULL, NULL, FALSE, FALSE,
                     _("Print"), gimp_print_callback,
                     NULL, NULL, NULL, TRUE, FALSE,
                     _("Cancel"), gtk_widget_destroy,
                     NULL, 1, NULL, FALSE, TRUE,

                     NULL);

  g_free (plug_in_name);

  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit),
                      NULL);

  /*
   * Top-level vbox for dialog...
   */

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), main_vbox,
                      FALSE, FALSE, 0);
  gtk_widget_show (main_vbox);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame = gtk_frame_new (_("Preview"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*
   * Drawing area for page preview...
   */

  preview = (GtkDrawingArea *) gtk_drawing_area_new ();
  gtk_drawing_area_size (preview, PREVIEW_SIZE_HORIZ, PREVIEW_SIZE_VERT);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (preview), FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (preview), "expose_event",
                      GTK_SIGNAL_FUNC (gimp_preview_update),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (preview), "button_press_event",
                      GTK_SIGNAL_FUNC (gimp_preview_button_callback),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (preview), "button_release_event",
                      GTK_SIGNAL_FUNC (gimp_preview_button_callback),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (preview), "motion_notify_event",
                      GTK_SIGNAL_FUNC (gimp_preview_motion_callback),
                      NULL);
  gtk_widget_show (GTK_WIDGET (preview));

  gtk_widget_set_events (GTK_WIDGET (preview),
                         GDK_EXPOSURE_MASK |
                         GDK_BUTTON_MOTION_MASK |
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK);

  table = gtk_table_new (4, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  recenter_button = button = gtk_button_new_with_label (_("Center Image"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_signal_connect(GTK_OBJECT (button), "clicked",
                     GTK_SIGNAL_FUNC (gimp_position_callback),
		     NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             NULL, 0, 0,
                             button, 2, TRUE);

  left_entry = entry = gtk_entry_new ();
  g_snprintf (s, sizeof (s), "%.3f", fabs (vars.left));
  gtk_entry_set_text (GTK_ENTRY (entry), s);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
                      GTK_SIGNAL_FUNC (gimp_position_callback),
		      NULL);
  gtk_widget_set_usize (entry, 60, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Left:"), 1.0, 0.5,
                             entry, 1, TRUE);

  top_entry = entry = gtk_entry_new ();
  g_snprintf (s, sizeof (s), "%.3f", fabs (vars.top));
  gtk_entry_set_text (GTK_ENTRY (entry), s);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
                      GTK_SIGNAL_FUNC (gimp_position_callback),
		      NULL);
  gtk_widget_set_usize (entry, 60, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 2, 1,
                             _("Top:"), 1.0, 0.5,
                             entry, 1, TRUE);

  right_entry = entry = gtk_entry_new ();
  g_snprintf (s, sizeof (s), "%.3f", fabs (vars.left));
  gtk_entry_set_text (GTK_ENTRY (entry), s);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
                      GTK_SIGNAL_FUNC (gimp_position_callback),
		      NULL);
  gtk_widget_set_usize (entry, 60, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("Right:"), 1.0, 0.5,
                             entry, 1, TRUE);

  bottom_entry = entry = gtk_entry_new ();
  g_snprintf (s, sizeof (s), "%.3f", fabs (vars.left));
  gtk_entry_set_text (GTK_ENTRY (entry), s);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
                      GTK_SIGNAL_FUNC (gimp_position_callback),
		      NULL);
  gtk_widget_set_usize (entry, 60, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 2, 2,
                             _("Bottom:"), 1.0, 0.5,
                             entry, 1, TRUE);

  width_entry = entry = gtk_entry_new ();
  g_snprintf (s, sizeof (s), "%.3f", fabs (vars.left));
  gtk_entry_set_text (GTK_ENTRY (entry), s);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
                      GTK_SIGNAL_FUNC (gimp_position_callback),
		      NULL);
  gtk_widget_set_usize (entry, 60, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
                             _("Width:"), 1.0, 0.5,
                             entry, 1, TRUE);

  height_entry = entry = gtk_entry_new ();
  g_snprintf (s, sizeof (s), "%.3f", fabs (vars.left));
  gtk_entry_set_text (GTK_ENTRY (entry), s);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
                      GTK_SIGNAL_FUNC (gimp_position_callback),
		      NULL);
  gtk_widget_set_usize (entry, 60, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 2, 3,
                             _("Height:"), 1.0, 0.5,
                             entry, 1, TRUE);

  /*
   *  Printer settings frame...
   */

  frame = gtk_frame_new (_("Printer Settings"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  table = printer_table = gtk_table_new (8, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*
   * Media size option menu...
   */

  media_size = option = gtk_option_menu_new ();
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Media Size:"), 1.0, 0.5,
                             option, 1, TRUE);

  /*
   * Media type option menu...
   */

  media_type = option = gtk_option_menu_new ();
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("Media Type:"), 1.0, 0.5,
                             option, 1, TRUE);

  /*
   * Media source option menu...
   */

  media_source = option = gtk_option_menu_new ();
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
                             _("Media Source:"), 1.0, 0.5,
                             option, 1, TRUE);

  /*
   * Ink type option menu...
   */

  ink_type = option = gtk_option_menu_new ();
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 4,
                             _("Ink Type:"), 1.0, 0.5,
                             option, 1, TRUE);

  /*
   * Orientation option menu...
   */

  option = gimp_option_menu_new (FALSE,
                                 _("Auto"), gimp_orientation_callback,
                                 (gpointer) -1, NULL, NULL,
				 vars.orientation == ORIENT_AUTO,
                                 _("Portrait"), gimp_orientation_callback,
                                 (gpointer) 0, NULL, NULL,
				 vars.orientation == ORIENT_PORTRAIT,
                                 _("Landscape"), gimp_orientation_callback,
                                 (gpointer) 1, NULL, NULL,
				 vars.orientation == ORIENT_LANDSCAPE,
                                 NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 5,
                             _("Orientation:"), 1.0, 0.5,
                             option, 1, TRUE);

  /*
   * Resolution option menu...
   */

  resolution = option = gtk_option_menu_new ();
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 6,
                             _("Resolution:"), 1.0, 0.5,
                             option, 1, TRUE);

  /*
   * Output type toggles...
   */

  box = gtk_vbox_new (FALSE, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 7,
                             _("Output Type:"), 1.0, 0.15,
                             box, 1, TRUE);

  output_gray = button = gtk_radio_button_new_with_label (NULL, _("B&W"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  if (vars.output_type == 0)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
                      GTK_SIGNAL_FUNC (gimp_output_type_callback),
                      (gpointer) OUTPUT_GRAY);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  output_color = button = gtk_radio_button_new_with_label (group, _("Color"));
  if (vars.output_type == 1)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
                      GTK_SIGNAL_FUNC (gimp_output_type_callback),
                      (gpointer) OUTPUT_COLOR);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

#ifdef DO_LINEAR
  box = gtk_vbox_new (FALSE, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 8,
                             _("Output Level:"), 1.0, 0.15,
                             box, 1, TRUE);

  linear_off = button =
    gtk_radio_button_new_with_label (NULL, _("Normal Scale"));
  linear_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  if (vars.linear == 0)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
                      GTK_SIGNAL_FUNC (gimp_linear_callback),
                      (gpointer) 0);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  linear_on = button =
    gtk_radio_button_new_with_label (linear_group,
				     _("Experimental Linear Scale"));
  if (vars.linear == 1)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
                      GTK_SIGNAL_FUNC (gimp_linear_callback),
                      (gpointer) 1);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
#endif

  /*
   * Image type
   */
  frame = gtk_frame_new (_("Image Type"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  box = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (box), 4);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (box);

  image_line_art = button =
    gtk_radio_button_new_with_label (NULL, _("Line Art"));
  image_type_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  if (vars.image_type == IMAGE_LINE_ART)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (gimp_image_type_callback),
		      (gpointer) IMAGE_LINE_ART);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  image_solid_tone= button =
    gtk_radio_button_new_with_label (image_type_group, _("Solid Colors"));
  image_type_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  if (vars.image_type == IMAGE_SOLID_TONE)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (gimp_image_type_callback),
		      (gpointer) IMAGE_SOLID_TONE);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  image_continuous_tone = button =
    gtk_radio_button_new_with_label (image_type_group, _("Photograph"));
  image_type_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  if (vars.image_type == IMAGE_CONTINUOUS)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (gimp_image_type_callback),
		      (gpointer) IMAGE_CONTINUOUS);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  image_monochrome= button =
    gtk_radio_button_new_with_label(image_type_group, _("Monochrome"));
  image_type_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  if (vars.image_type == IMAGE_MONOCHROME)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (gimp_image_type_callback),
		      (gpointer) IMAGE_MONOCHROME);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*
   * Scaling...
   */
  frame = gtk_frame_new (_("Scaling and Color Settings"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  if (vars.scaling < 0.0)
    {
      scaling_adjustment =
        gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("Scaling:"), 200, 0,
                              -vars.scaling, 36.0, 1200.0, 1.0, 10.0, 1,
                              TRUE, 0, 0,
                              NULL, NULL);
    }
  else
    {
      scaling_adjustment =
        gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("Scaling:"), 200, 75,
                              vars.scaling, 5.0, 100.0, 1.0, 10.0, 1,
                              TRUE, 0, 0,
                              NULL, NULL);
    }
  gtk_signal_connect (GTK_OBJECT (scaling_adjustment), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_scaling_update),
                      NULL);

  box = gtk_hbox_new (FALSE, 4);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             NULL, 0, 0,
                             box, 1, FALSE);

  scaling_percent = button =
    gtk_radio_button_new_with_label (NULL, _("Percent"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  if (vars.scaling > 0.0)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
                      GTK_SIGNAL_FUNC (gimp_scaling_callback),
                      NULL);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  scaling_ppi = button = gtk_radio_button_new_with_label (group, _("PPI"));
  if (vars.scaling < 0.0)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
                      GTK_SIGNAL_FUNC (gimp_scaling_callback),
                      NULL);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  scaling_image = button = gtk_button_new_with_label (_("Set Image Scale"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (gimp_scaling_callback),
                      NULL);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*
   *  Color adjust button
   */
  gimp_create_color_adjust_window ();

  adjust_color_button = button = gtk_button_new_with_label (_("Adjust Color"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_show),
			     GTK_OBJECT (gimp_color_adjust_dialog));
  gtk_box_pack_end (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*
   * Printer option menu...
   */

  menu = gtk_menu_new ();
  for (i = 0; i < plist_count; i ++)
  {
    if (plist[i].active)
      item = gtk_menu_item_new_with_label (gettext (plist[i].name));
    else
      {
        gchar buf[18];
        buf[0] = '*';
        memcpy (buf + 1, plist[i].name, 17);
        item = gtk_menu_item_new_with_label (gettext (buf));
      }
    gtk_menu_append (GTK_MENU (menu), item);
    gtk_signal_connect (GTK_OBJECT (item), "activate",
                        GTK_SIGNAL_FUNC (gimp_plist_callback),
                        (gpointer) i);
    gtk_widget_show (item);
  }

  box = gtk_hbox_new (FALSE, 6);
  gimp_table_attach_aligned (GTK_TABLE (printer_table), 0, 0,
                             _("Printer:"), 1.0, 0.5,
                             box, 1, TRUE);

  option = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (box), option, FALSE, FALSE, 0);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option), menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (option), plist_current);
  gtk_widget_show (option);

  button = gtk_button_new_with_label (_("Setup"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (gimp_setup_open_callback),
                      NULL);
  gtk_widget_show (button);

  /*
   * Setup dialog window...
   */

  setup_dialog = dialog =
    gimp_dialog_new (_("Setup"), "print",
                     gimp_plugin_help_func, "filters/print.html",
                     GTK_WIN_POS_MOUSE,
                     FALSE, TRUE, FALSE,

                     _("OK"), gimp_setup_ok_callback,
                     NULL, NULL, NULL, TRUE, FALSE,
                     _("Cancel"), gtk_widget_hide,
                     NULL, 1, NULL, FALSE, TRUE,

                     NULL);

  /*
   * Top-level table for dialog...
   */

  table = gtk_table_new (3, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 8);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), table,
                      FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*
   * Printer driver option menu...
   */

  label = gtk_label_new (_("Driver:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  menu = gtk_menu_new ();
  for (i = 0; i < known_printers (); i ++)
    {
      if (!strcmp (the_printer->long_name, ""))
	continue;

      item = gtk_menu_item_new_with_label (gettext (the_printer->long_name));
      gtk_menu_append (GTK_MENU (menu), item);
      gtk_signal_connect (GTK_OBJECT (item), "activate",
                          GTK_SIGNAL_FUNC (gimp_print_driver_callback),
                          (gpointer) i);
      gtk_widget_show (item);
      the_printer++;
    }

  printer_driver = option = gtk_option_menu_new ();
  gtk_table_attach (GTK_TABLE (table), option, 1, 2, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option), menu);
  gtk_widget_show (option);

  /*
   * PPD file...
   */

  label = gtk_label_new (_("PPD File:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  box = gtk_hbox_new (FALSE, 8);
  gtk_table_attach (GTK_TABLE (table), box, 1, 2, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (box);

  ppd_file = entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (box), entry, TRUE, TRUE, 0);
  gtk_widget_show (entry);

  ppd_button = button = gtk_button_new_with_label (_("Browse"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (gimp_ppd_browse_callback),
                      NULL);
  gtk_widget_show (button);

  /*
   * Print command...
   */

  label = gtk_label_new (_("Command:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  output_cmd = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (entry);

  /*
   * Output file selection dialog...
   */

  file_browser = gtk_file_selection_new (_("Print To File?"));
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_browser)->ok_button),
                      "clicked",
                      GTK_SIGNAL_FUNC (gimp_file_ok_callback),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_browser)->cancel_button),
                      "clicked",
		      GTK_SIGNAL_FUNC (gimp_file_cancel_callback),
                      NULL);

  /*
   * PPD file selection dialog...
   */

  ppd_browser = gtk_file_selection_new (_("PPD File?"));
  gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (ppd_browser));
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (ppd_browser)->ok_button),
                      "clicked",
                      GTK_SIGNAL_FUNC (gimp_ppd_ok_callback),
                      NULL);
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (ppd_browser)->cancel_button),
                             "clicked",
                             GTK_SIGNAL_FUNC (gtk_widget_hide),
                             GTK_OBJECT (ppd_browser));

  /*
   * Show the main dialog and wait for the user to do something...
   */

  gimp_plist_callback (NULL, (gpointer) plist_current);

  gtk_widget_show (print_dialog);
}

/*
 *  gimp_scaling_update() - Update the scaling scale using the slider.
 */
static void
gimp_scaling_update (GtkAdjustment *adjustment)
{
  if (vars.scaling != adjustment->value)
    {
      if (GTK_TOGGLE_BUTTON (scaling_ppi)->active)
        vars.scaling = -adjustment->value;
      else
        vars.scaling = adjustment->value;
      plist[plist_current].v.scaling = vars.scaling;
    }

  gimp_preview_update ();
}

/*
 *  gimp_scaling_callback() - Update the scaling scale using radio buttons.
 */
static void
gimp_scaling_callback (GtkWidget *widget)
{
  if (widget == scaling_ppi)
    {
      GTK_ADJUSTMENT (scaling_adjustment)->lower = 36.0;
      GTK_ADJUSTMENT (scaling_adjustment)->upper = 1200.0;
      GTK_ADJUSTMENT (scaling_adjustment)->value = 72.0;
      vars.scaling = 0.0;
      plist[plist_current].v.scaling = vars.scaling;
    }
  else if (widget == scaling_percent)
    {
      GTK_ADJUSTMENT (scaling_adjustment)->lower = 5.0;
      GTK_ADJUSTMENT (scaling_adjustment)->upper = 100.0;
      GTK_ADJUSTMENT (scaling_adjustment)->value = 100.0;
      vars.scaling = 0.0;
      plist[plist_current].v.scaling = vars.scaling;
    }
  else if (widget == scaling_image)
    {
      gdouble xres, yres;

      gimp_image_get_resolution (image_ID, &xres, &yres);

      GTK_ADJUSTMENT (scaling_adjustment)->lower = 36.0;
      GTK_ADJUSTMENT (scaling_adjustment)->upper = 1200.0;
      GTK_ADJUSTMENT (scaling_adjustment)->value = yres;
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scaling_ppi), TRUE);
      vars.scaling = 0.0;
      plist[plist_current].v.scaling = vars.scaling;
    }

  gtk_adjustment_changed (GTK_ADJUSTMENT (scaling_adjustment));
  gtk_adjustment_value_changed (GTK_ADJUSTMENT (scaling_adjustment));
}

/*
 *  gimp_plist_build_menu ()
 */
static void
gimp_plist_build_menu (GtkWidget      *option,
		       GtkWidget     **menu,
		       gint            num_items,
		       gchar         **items,
		       gchar          *cur_item,
		       GtkSignalFunc   callback)
{
  gint       i;
  GtkWidget *item;
  GtkWidget *item0 = NULL;

  if (*menu != NULL)
    {
      gtk_widget_destroy (*menu);
      *menu = NULL;
    }

  *menu = gtk_menu_new ();

  if (num_items == 0)
    {
      item = gtk_menu_item_new_with_label (_("Standard"));
      gtk_menu_append (GTK_MENU (*menu), item);
      gtk_widget_show (item);
      gtk_option_menu_set_menu (GTK_OPTION_MENU (option), *menu);
      gtk_widget_set_sensitive (option, FALSE);
      gtk_widget_show (option);
      return;
    }
  else
    {
      gtk_widget_set_sensitive (option, TRUE);
    }

  for (i = 0; i < num_items; i ++)
    {
      item = gtk_menu_item_new_with_label (gettext (items[i]));
      if (i == 0)
	item0 = item;
      gtk_menu_append (GTK_MENU (*menu), item);
      gtk_signal_connect (GTK_OBJECT (item), "activate",
			  callback,
			  (gpointer) i);
      gtk_widget_show (item);
    }

  gtk_option_menu_set_menu (GTK_OPTION_MENU (option), *menu);

#ifdef DEBUG
  printf ("cur_item = \'%s\'\n", cur_item);
#endif /* DEBUG */

  for (i = 0; i < num_items; i ++)
    {
#ifdef DEBUG
      printf ("item[%d] = \'%s\'\n", i, items[i]);
#endif /* DEBUG */

      if (strcmp (items[i], cur_item) == 0)
	{
	  gtk_option_menu_set_history (GTK_OPTION_MENU (option), i);
	  break;
	}
    }

  if (i == num_items)
    {
      gtk_option_menu_set_history (GTK_OPTION_MENU (option), 0);
      gtk_signal_emit_by_name (GTK_OBJECT (item0), "activate");
    }
}

/*
 *  gimp_do_misc_updates() - Build an option menu for the given parameters...
 */
static void
gimp_do_misc_updates (void)
{
  vars.scaling = plist[plist_current].v.scaling;
  vars.orientation = plist[plist_current].v.orientation;
  vars.left = plist[plist_current].v.left;
  vars.top = plist[plist_current].v.top;

  if (plist[plist_current].v.scaling < 0)
    {
      float tmp = -plist[plist_current].v.scaling;
      plist[plist_current].v.scaling = -plist[plist_current].v.scaling;
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scaling_ppi), TRUE);
      GTK_ADJUSTMENT (scaling_adjustment)->lower = 36.0;
      GTK_ADJUSTMENT (scaling_adjustment)->upper = 1200.0;
      GTK_ADJUSTMENT (scaling_adjustment)->value = tmp;
      gtk_signal_emit_by_name (scaling_adjustment, "changed");
      gtk_signal_emit_by_name (scaling_adjustment, "value_changed");
    }
  else
    {
      float tmp = plist[plist_current].v.scaling;
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scaling_percent), TRUE);
      GTK_ADJUSTMENT (scaling_adjustment)->lower = 5.0;
      GTK_ADJUSTMENT (scaling_adjustment)->upper = 100.0;
      GTK_ADJUSTMENT (scaling_adjustment)->value = tmp;
      gtk_signal_emit_by_name (scaling_adjustment, "changed");
      gtk_signal_emit_by_name (scaling_adjustment, "value_changed");
    }

  gtk_adjustment_set_value (GTK_ADJUSTMENT (brightness_adjustment),
			    plist[plist_current].v.brightness);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (gamma_adjustment),
			    plist[plist_current].v.gamma);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (contrast_adjustment),
			    plist[plist_current].v.contrast);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (red_adjustment),
			    plist[plist_current].v.red);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (green_adjustment),
			    plist[plist_current].v.green);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (blue_adjustment),
			    plist[plist_current].v.blue);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (saturation_adjustment),
			    plist[plist_current].v.saturation);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (density_adjustment),
			    plist[plist_current].v.density);

  if (plist[plist_current].v.output_type == OUTPUT_GRAY)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (output_gray), TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (output_color), TRUE);

#ifdef DO_LINEAR
  if (plist[plist_current].v.linear == 0)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linear_off), TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linear_on), TRUE);
#endif

  switch (plist[plist_current].v.image_type)
    {
    case IMAGE_LINE_ART:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (image_line_art), TRUE);
      break;
    case IMAGE_SOLID_TONE:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (image_solid_tone), TRUE);
      break;
    case IMAGE_CONTINUOUS:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (image_continuous_tone), TRUE);
      break;
    case IMAGE_MONOCHROME:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (image_monochrome), TRUE);
      break;
    default:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (image_continuous_tone), TRUE);
      plist[plist_current].v.image_type = IMAGE_CONTINUOUS;
      break;
    }

  gimp_preview_update ();
}

/*
 * gimp_position_callback() - callback for position entry widgets
 */
static void
gimp_position_callback (GtkWidget *widget)
{
  gboolean dontcheck = FALSE;

  if (widget == top_entry)
    {
      gfloat new_value = atof (gtk_entry_get_text (GTK_ENTRY (widget)));
      vars.top = ((new_value + 1.0 / 144) * 72) - top;
    }
  else if (widget == left_entry)
    {
      gfloat new_value = atof (gtk_entry_get_text (GTK_ENTRY (widget)));
      vars.left = ((new_value + 1.0 / 144) * 72) - left;
    }
  else if (widget == bottom_entry)
    {
      gfloat new_value = atof (gtk_entry_get_text (GTK_ENTRY (widget)));
      vars.top = ((new_value + 1.0 / 144) * 72) - (top + print_height); 
    }
  else if (widget == right_entry)
    {
      gfloat new_value = atof (gtk_entry_get_text (GTK_ENTRY (widget)));
      vars.left = ((new_value + 1.0 / 144) * 72) - (left + print_width);
    }
  else if (widget == width_entry)
    {
      gfloat new_value = atof (gtk_entry_get_text (GTK_ENTRY (widget)));
      if (vars.scaling >= 0) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scaling_ppi), TRUE);
        gimp_scaling_callback (scaling_ppi);
      }
      GTK_ADJUSTMENT (scaling_adjustment)->value = image_width / new_value;
      gtk_signal_emit_by_name (scaling_adjustment, "value_changed");
    }
  else if (widget == height_entry)
    {
      gfloat new_value = atof (gtk_entry_get_text (GTK_ENTRY (widget)));
      if (vars.scaling >= 0) {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scaling_ppi), TRUE);
        gimp_scaling_callback (scaling_ppi);
      }
      GTK_ADJUSTMENT (scaling_adjustment)->value = image_height / new_value;
      gtk_signal_emit_by_name (scaling_adjustment, "value_changed");
    }
  else if (widget == recenter_button)
    {
      vars.left = -1;
      vars.top = -1;
      dontcheck = TRUE;
    }
  if (!dontcheck)
    {
      if (vars.left < 0)
	vars.left = 0;
      if (vars.top < 0)
	vars.top = 0;
    }
  plist[plist_current].v.left = vars.left;
  plist[plist_current].v.top = vars.top;
  gimp_preview_update ();
}

/*
 *  gimp_plist_callback() - Update the current system printer...
 */
static void
gimp_plist_callback (GtkWidget *widget,
		     gpointer   data)
{
  gint     i;
  plist_t *p;

  plist_current = (gint) data;
  p             = plist + plist_current;

  if (p->v.driver[0] != '\0')
    {
      strcpy(vars.driver, p->v.driver);

      current_printer = get_printer_by_driver (vars.driver);
    }

  strcpy (vars.ppd_file, p->v.ppd_file);
  strcpy (vars.media_size, p->v.media_size);
  strcpy (vars.media_type, p->v.media_type);
  strcpy (vars.media_source, p->v.media_source);
  strcpy (vars.ink_type, p->v.ink_type);
  strcpy (vars.dither_algorithm, p->v.dither_algorithm);
  strcpy (vars.resolution, p->v.resolution);
  strcpy (vars.output_to, p->v.output_to);

  gimp_do_misc_updates ();

  /*
   * Now get option parameters...
   */

  if (num_media_sizes > 0)
    {
      for (i = 0; i < num_media_sizes; i ++)
	free (media_sizes[i]);
      free (media_sizes);
  }

  media_sizes = (*(current_printer->parameters))(current_printer->model,
						 p->v.ppd_file,
						 "PageSize", &num_media_sizes);
  if (vars.media_size[0] == '\0')
    strcpy (vars.media_size, media_sizes[0]);
  gimp_plist_build_menu (media_size,
			 &media_size_menu,
			 num_media_sizes,
			 media_sizes,
			 p->v.media_size,
			 gimp_media_size_callback);

  if (num_media_types > 0)
    {
      for (i = 0; i < num_media_types; i ++)
	free (media_types[i]);
      free (media_types);
    }

  media_types = (*(current_printer->parameters))(current_printer->model,
						 p->v.ppd_file,
						 "MediaType",
						 &num_media_types);
  if (vars.media_type[0] == '\0' && media_types != NULL)
    strcpy (vars.media_type, media_types[0]);
  else if (media_types == NULL)
    vars.media_type[0] = '\0';
  gimp_plist_build_menu (media_type,
			 &media_type_menu,
			 num_media_types,
			 media_types,
			 p->v.media_type,
			 gimp_media_type_callback);

  if (num_media_sources > 0)
    {
      for (i = 0; i < num_media_sources; i ++)
	free (media_sources[i]);
      free (media_sources);
    }

  media_sources = (*(current_printer->parameters))(current_printer->model,
						   p->v.ppd_file,
						   "InputSlot",
						   &num_media_sources);
  if (vars.media_source[0] == '\0' && media_sources != NULL)
    strcpy (vars.media_source, media_sources[0]);
  else if (media_sources == NULL)
    vars.media_source[0] = '\0';
  gimp_plist_build_menu (media_source,
			 &media_source_menu,
			 num_media_sources,
			 media_sources,
			 p->v.media_source,
			 gimp_media_source_callback);

  if (num_ink_types > 0)
    {
      for (i = 0; i < num_ink_types; i ++)
	free (ink_types[i]);
      free (ink_types);
    }

  ink_types = (*(current_printer->parameters))(current_printer->model,
					       p->v.ppd_file,
					       "InkType", &num_ink_types);
  if (vars.ink_type[0] == '\0' && ink_types != NULL)
    strcpy (vars.ink_type, ink_types[0]);
  else if (ink_types == NULL)
    vars.ink_type[0] = '\0';
  gimp_plist_build_menu (ink_type,
			 &ink_type_menu,
			 num_ink_types,
			 ink_types,
			 p->v.ink_type,
			 gimp_ink_type_callback);

  if (num_resolutions > 0)
    {
      for (i = 0; i < num_resolutions; i ++)
	free (resolutions[i]);
      free (resolutions);
    }

  resolutions = (*(current_printer->parameters))(current_printer->model,
						 p->v.ppd_file,
						 "Resolution",
						 &num_resolutions);
  if (vars.resolution[0] == '\0' && resolutions != NULL)
    strcpy (vars.resolution, resolutions[0]);
  else if (resolutions == NULL)
    vars.resolution[0] = '\0';
  gimp_plist_build_menu (resolution,
			 &resolution_menu,
			 num_resolutions,
			 resolutions,
			 p->v.resolution,
			 gimp_resolution_callback);
}

/*
 *  gimp_media_size_callback() - Update the current media size...
 */
static void
gimp_media_size_callback(GtkWidget *widget,
			 gpointer   data)
{
  strcpy (vars.media_size, media_sizes[(gint) data]);
  strcpy (plist[plist_current].v.media_size, media_sizes[(gint) data]);
  vars.left = -1;
  vars.top  = -1;
  plist[plist_current].v.left = vars.left;
  plist[plist_current].v.top = vars.top;

  gimp_preview_update ();
}

/*
 *  gimp_media_type_callback() - Update the current media type...
 */
static void
gimp_media_type_callback (GtkWidget *widget,
			  gpointer   data)
{
  strcpy (vars.media_type, media_types[(gint) data]);
  strcpy (plist[plist_current].v.media_type, media_types[(gint) data]);
}

/*
 *  gimp_media_source_callback() - Update the current media source...
 */
static void
gimp_media_source_callback (GtkWidget *widget,
			    gpointer   data)
{
  strcpy (vars.media_source, media_sources[(gint) data]);
  strcpy (plist[plist_current].v.media_source, media_sources[(gint) data]);
}

/*
 *  gimp_ink_type_callback() - Update the current media source...
 */
static void
gimp_ink_type_callback (GtkWidget *widget,
			gpointer   data)
{
  strcpy (vars.ink_type, ink_types[(gint) data]);
  strcpy (plist[plist_current].v.ink_type, ink_types[(gint) data]);
}

/*
 *  gimp_resolution_callback() - Update the current resolution...
 */
static void
gimp_resolution_callback (GtkWidget *widget,
			  gpointer   data)
{
  strcpy (vars.resolution, resolutions[(gint) data]);
  strcpy (plist[plist_current].v.resolution, resolutions[(gint) data]);
}

/*
 *  gimp_orientation_callback() - Update the current media size...
 */
static void
gimp_orientation_callback (GtkWidget *widget,
			   gpointer   data)
{
  vars.orientation = (gint) data;
  vars.left        = -1;
  vars.top         = -1;
  plist[plist_current].v.orientation = vars.orientation;
  plist[plist_current].v.left = vars.left;
  plist[plist_current].v.top = vars.top;

  gimp_preview_update ();
}

/*
 *  gimp_output_type_callback() - Update the current output type...
 */
static void
gimp_output_type_callback (GtkWidget *widget,
			   gpointer   data)
{
  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      vars.output_type = (gint) data;
      plist[plist_current].v.output_type = (gint) data;
    }
}

/*
 *  gimp_linear_callback() - Update the current linear gradient mode...
 */
#ifdef DO_LINEAR
static void
gimp_linear_callback (GtkWidget *widget,
		      gpointer   data)
{
  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      vars.linear = (gint) data;
      plist[plist_current].v.linear = (gint) data;
    }
}
#endif

/*
 *  gimp_image_type_callback() - Update the current linear gradient mode...
 */
static void
gimp_image_type_callback (GtkWidget *widget,
			  gpointer   data)
{
  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      vars.image_type = (gint) data;
      plist[plist_current].v.image_type = (gint) data;
    }
}

/*
 * 'print_callback()' - Start the print...
 */
static void
gimp_print_callback (void)
{
  if (plist_current > 0)
    {
      runme = TRUE;

      gtk_widget_destroy (print_dialog);
    }
  else
    {
      gtk_widget_set_sensitive (print_dialog, FALSE);
      gtk_widget_show (file_browser);
    }
}

/*
 *  gimp_printandsave_callback() - 
 */
static void
gimp_printandsave_callback (void)
{
  saveme = TRUE;

  if (plist_current > 0)
    {
      runme = TRUE;

      gtk_widget_destroy (print_dialog);
    }
  else
    {
      gtk_widget_set_sensitive (print_dialog, FALSE);
      gtk_widget_show (file_browser);
    }
}

/*
 *  gimp_save_callback() - save settings, don't destroy dialog
 */
static void
gimp_save_callback (void)
{
  printrc_save ();
}

/*
 *  gimp_setup_open__callback() - 
 */
static void
gimp_setup_open_callback (void)
{
  gint idx;

  current_printer = get_printer_by_driver (plist[plist_current].v.driver);
  idx = get_printer_index_by_driver (plist[plist_current].v.driver);

  gtk_option_menu_set_history (GTK_OPTION_MENU (printer_driver), idx);

  gtk_entry_set_text (GTK_ENTRY (ppd_file), plist[plist_current].v.ppd_file);

  if (strncmp (plist[plist_current].v.driver, "ps", 2) == 0)
    {
      gtk_widget_show (ppd_file);
      gtk_widget_show (ppd_button);
    }
  else
    {
      gtk_widget_hide (ppd_file);
      gtk_widget_hide (ppd_button);
    }

  gtk_entry_set_text (GTK_ENTRY (output_cmd), plist[plist_current].v.output_to);

  if (plist_current == 0)
    gtk_widget_hide (output_cmd);
  else
    gtk_widget_show (output_cmd);

  gtk_widget_show (setup_dialog);
}

/*
 *  gimp_setup_ok_callback() - 
 */
static void
gimp_setup_ok_callback (void)
{
  strcpy (vars.driver, current_printer->driver);
  strcpy (plist[plist_current].v.driver, current_printer->driver);

  strcpy (vars.output_to, gtk_entry_get_text (GTK_ENTRY (output_cmd)));
  strcpy (plist[plist_current].v.output_to, vars.output_to);

  strcpy (vars.ppd_file, gtk_entry_get_text (GTK_ENTRY (ppd_file)));
  strcpy (plist[plist_current].v.ppd_file, vars.ppd_file);

  gimp_plist_callback (NULL, (gpointer) plist_current);

  gtk_widget_hide (setup_dialog);
}

/*
 *  gimp_print_driver_callback() - Update the current printer driver...
 */
static void
gimp_print_driver_callback (GtkWidget *widget,
			    gpointer   data)
{
  current_printer = get_printer_by_index ((gint) data);

  if (strncmp (current_printer->driver, "ps", 2) == 0)
    {
      gtk_widget_show (ppd_file);
      gtk_widget_show (ppd_button);
    }
  else
    {
      gtk_widget_hide (ppd_file);
      gtk_widget_hide (ppd_button);
    }
}

/*
 *  gimp_ppd_browse_callback() - 
 */
static void
gimp_ppd_browse_callback (void)
{
  gtk_file_selection_set_filename (GTK_FILE_SELECTION (ppd_browser),
				   gtk_entry_get_text (GTK_ENTRY (ppd_file)));
  gtk_widget_show (ppd_browser);
}

/*
 *  gimp_ppd_ok_callback() - 
 */
static void
gimp_ppd_ok_callback (void)
{
  gtk_widget_hide (ppd_browser);
  gtk_entry_set_text
    (GTK_ENTRY (ppd_file),
     gtk_file_selection_get_filename (GTK_FILE_SELECTION (ppd_browser)));
}

/*
 *  gimp_file_ok_callback() - print to file and go away
 */
static void
gimp_file_ok_callback (void)
{
  gtk_widget_hide (file_browser);
  strcpy (vars.output_to,
	  gtk_file_selection_get_filename (GTK_FILE_SELECTION (file_browser)));

  runme = TRUE;

  gtk_widget_destroy (print_dialog);
}

/*
 *  gimp_file_cancel_callback() - 
 */
static void
gimp_file_cancel_callback (void)
{
  gtk_widget_hide (file_browser);

  gtk_widget_set_sensitive (print_dialog, TRUE);
}

/*
 *  gimp_preview_update_callback() - 
 */
static void
gimp_preview_update (void)
{
  gint	        temp;
  gint          orient;		   /* True orientation of page */
  gdouble	min_ppi_scaling;   /* Minimum PPI for current page size */
  static GdkGC	*gc = NULL,
		*gcinv = NULL;
  gchar         s[255];
  gint          paper_left, paper_top;

  if (preview->widget.window == NULL)
    return;

  gdk_window_clear (preview->widget.window);

  if (gc == NULL)
  {
    gc = gdk_gc_new (preview->widget.window);
    gcinv = gdk_gc_new(preview->widget.window);
    gdk_gc_set_function (gcinv, GDK_INVERT);
  }


  (*current_printer->media_size) (current_printer->model, vars.ppd_file,
				  vars.media_size, &paper_width, &paper_height);

  (*current_printer->imageable_area) (current_printer->model, vars.ppd_file,
				      vars.media_size, &left, &right,
				      &bottom, &top);

  /* Rationalise things a bit by measuring everything from the top left */
  top = paper_height - top;
  bottom = paper_height - bottom;

  printable_width  = right - left;
  printable_height = bottom - top;

  if (vars.orientation == ORIENT_AUTO)
  {
    if ((printable_width >= printable_height && image_width >= image_height)
	|| (printable_height >= printable_width && image_height >= image_width))
      orient = ORIENT_PORTRAIT;
    else
      orient = ORIENT_LANDSCAPE;
  }
  else
    orient = vars.orientation;

  if (orient == ORIENT_LANDSCAPE)
    {
      /*
       * In landscape mode, we print the right-hand side of the logical
       * page at the top of the physical paper.  So, left-to-right on
       * the paper corresponds to top-to-bottom on the image, so we
       * don't have to flip the columns of the image end-for-end while
       * forming rows for printing.
       */
      temp              = printable_width;
      printable_width   = printable_height;
      printable_height  = temp;
      temp              = paper_width;
      paper_width       = paper_height;
      paper_height      = temp;
      temp              = left;
      left              = paper_width - bottom;
      bottom            = right;
      right             = paper_width - top;
      top               = temp;
    }

  if (orient == ORIENT_PORTRAIT)
    min_ppi_scaling = 72.0 * (double) image_width / (double) printable_width;
  else
    min_ppi_scaling = 72.0 * (double) image_height / (double) printable_height;

  if (vars.scaling < -min_ppi_scaling)
    vars.scaling = -min_ppi_scaling;

  if (vars.scaling < 0)
  {
    print_width = 72 * -image_width / vars.scaling;
    print_height = print_width * image_height / image_width;
  }
  else
  {
    /* we do vars.scaling % of height or width, whatever is smaller */
    /* this is relative to printable size */
    if (image_width * printable_height > printable_width * image_height)
      /* i.e. if image_width/image_height > printable_width/printable_height */
      /* i.e. if image is wider relative to its height than the width
	 of the printable area relative to its height */
    {
      print_width = printable_width * vars.scaling / 100;
      print_height = print_width * image_height / image_width;
    }
    else
    {
      print_height = printable_height * vars.scaling / 100;
      print_width = print_height * image_width / image_height;
    }
  }

  paper_left = (PREVIEW_SIZE_HORIZ - PREVIEW_PPI * paper_width / 72) / 2;
  paper_top  = (PREVIEW_SIZE_VERT - PREVIEW_PPI * paper_height / 72) / 2;
  printable_left = paper_left +  PREVIEW_PPI * left / 72;
  printable_top  = paper_top + PREVIEW_PPI * top / 72 ;

  /* draw paper frame */ 
  gdk_draw_rectangle(preview->widget.window, gc, 0, 
 		     paper_left, paper_top,
		     PREVIEW_PPI * paper_width / 72,
		     PREVIEW_PPI * paper_height / 72);

  /* draw printable frame */
  gdk_draw_rectangle(preview->widget.window, gc, 0,
                     printable_left, printable_top,
                     PREVIEW_PPI * printable_width / 72,
                     PREVIEW_PPI * printable_height / 72);

  if (vars.left < 0)
    vars.left = (paper_width - print_width) / 2;

  /* we leave vars.left etc. relative to printable area */
  if (vars.left > (printable_width - print_width))
    vars.left = printable_width - print_width;


  if (vars.top < 0)
    vars.top  = ((paper_height - print_height) / 2) - top;

 
  if (vars.top > (printable_height - print_height))
    vars.top = printable_height - print_height; 
  
  plist[plist_current].v.left = vars.left;
  plist[plist_current].v.top = vars.top;

  g_snprintf (s, sizeof (s), "%.2f", (top + vars.top) / 72.0);
  gtk_signal_handler_block_by_data (GTK_OBJECT (top_entry), NULL);
  gtk_entry_set_text (GTK_ENTRY (top_entry), s);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (top_entry), NULL);

  g_snprintf (s, sizeof (s), "%.2f",(left + vars.left) / 72.0);
  gtk_signal_handler_block_by_data (GTK_OBJECT (left_entry), NULL);
  gtk_entry_set_text (GTK_ENTRY (left_entry), s);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (left_entry), NULL);

  gtk_signal_handler_block_by_data (GTK_OBJECT (bottom_entry), NULL);
  g_snprintf(s, sizeof (s), "%.2f", 
	     (top + vars.top + print_height) / 72.0);
  gtk_entry_set_text (GTK_ENTRY (bottom_entry), s);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (bottom_entry), NULL);

  gtk_signal_handler_block_by_data (GTK_OBJECT (right_entry), NULL);
  g_snprintf (s, sizeof (s), "%.2f", 
	      (left + vars.left + print_width) / 72.0);

  gtk_entry_set_text (GTK_ENTRY (right_entry), s);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (right_entry), NULL);

  gtk_signal_handler_block_by_data (GTK_OBJECT (width_entry), NULL);
  g_snprintf (s, sizeof (s), "%.2f", print_width / 72.0);
  gtk_entry_set_text (GTK_ENTRY (width_entry), s);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (width_entry), NULL);

  gtk_signal_handler_block_by_data (GTK_OBJECT (height_entry), NULL);
  g_snprintf(s, sizeof (s), "%.2f", print_height / 72.0);
  gtk_entry_set_text (GTK_ENTRY (height_entry), s);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (height_entry), NULL);

  /* draw image */
  gdk_draw_rectangle(preview->widget.window, gc, 1,
		     1 + printable_left + PREVIEW_PPI * vars.left / 72, 
		     1 + printable_top + PREVIEW_PPI * vars.top / 72,
                     PREVIEW_PPI * print_width / 72,
                     PREVIEW_PPI * print_height / 72);

  /* draw orientation arrow pointing to top-of-paper */
  {
    int ox, oy, u;
    if (paper_width < paper_height)
      u = PREVIEW_PPI * paper_width / 72 / 4;
    else
      u = PREVIEW_PPI * paper_height / 72 / 4;
    ox = paper_left + PREVIEW_PPI * paper_width / 72;
    oy = paper_top;
    if (orient == ORIENT_LANDSCAPE) {
      ox -= u;
      oy += PREVIEW_PPI * paper_height / 72 / 2;
      gdk_draw_line (preview->widget.window, gcinv, ox, oy, ox-u, oy-u);
      gdk_draw_line (preview->widget.window, gcinv, ox, oy, ox-u, oy+u);
      gdk_draw_line (preview->widget.window, gcinv, ox, oy, ox-2*u, oy);
    } else {
      ox -= PREVIEW_PPI * paper_width / 72 / 2;
      oy += u;
      gdk_draw_line (preview->widget.window, gcinv, ox, oy, ox-u, oy+u);
      gdk_draw_line (preview->widget.window, gcinv, ox, oy, ox+u, oy+u);
      gdk_draw_line (preview->widget.window, gcinv, ox, oy, ox, oy+2*u);
    }
  }

  gdk_flush ();
}

/*
 *  gimp_preview_button_callback() - 
 */
static void
gimp_preview_button_callback (GtkWidget      *widget,
			      GdkEventButton *event,
			      gpointer        data)
{
  mouse_x = event->x;
  mouse_y = event->y;
}

/*
 *  gimp_preview_motion_callback() - 
 */
static void
gimp_preview_motion_callback (GtkWidget      *widget,
			      GdkEventMotion *event,
			      gpointer        data)
{
  if (vars.left < 0 || vars.top < 0)
    {
      vars.left = 72 * (printable_width - print_width) / 20;
      vars.top  = 72 * (printable_height - print_height) / 20;
    }

  vars.left += 72 * (event->x - mouse_x) / PREVIEW_PPI;
  vars.top  += 72 * (event->y - mouse_y) / PREVIEW_PPI;

  if (vars.left < 0)
    vars.left = 0;

  if (vars.top < 0)
    vars.top = 0;
  plist[plist_current].v.left = vars.left;
  plist[plist_current].v.top = vars.top;

  gimp_preview_update ();

  mouse_x = event->x;
  mouse_y = event->y;
}

#endif  /* ! GIMP_1_0 */
