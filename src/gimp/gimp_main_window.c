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

#define MAX_PREVIEW_PPI        (30)

#include "print_gimp.h"

#include "print-intl.h"
#include <string.h>

/*
 * Constants for GUI...
 */
#define PREVIEW_SIZE_VERT  320 /* Assuming max media size of 24" A2 */
#define PREVIEW_SIZE_HORIZ 280 /* Assuming max media size of 24" A2 */

/*
 *  Main window widgets
 */
static GtkWidget *print_dialog;           /* Print dialog window */
static GtkWidget *recenter_button;
static GtkWidget *left_entry;
static GtkWidget *right_entry;
static GtkWidget *right_border_entry;
static GtkWidget *width_entry;
static GtkWidget *top_entry;
static GtkWidget *bottom_entry;
static GtkWidget *bottom_border_entry;
static GtkWidget *height_entry;
static GtkWidget *unit_inch;
static GtkWidget *unit_cm;
static GtkWidget *media_size_combo         = NULL;  /* Media size combo box */
static GtkWidget *custom_size_width        = NULL;
static GtkWidget *custom_size_height       = NULL;
static gint       media_size_callback_id   = -1;
static GtkWidget *media_type_combo         = NULL;  /* Media type combo box */
static gint       media_type_callback_id   = -1;    /* Media type calback ID */
static GtkWidget *media_source_combo       = NULL;  /* Media source combo box */
static gint       media_source_callback_id = -1;    /* Media source calback ID */
static GtkWidget *ink_type_combo           = NULL;  /* Ink type combo box */
static gint       ink_type_callback_id     = -1;    /* Ink type calback ID */
static GtkWidget *resolution_combo         = NULL;  /* Resolution combo box */
static gint       resolution_callback_id   = -1;    /* Resolution calback ID */
static GtkWidget *orientation_menu         = NULL;  /* Orientation menu */
static GtkWidget *scaling_percent;        /* Scale by percent */
static GtkWidget *scaling_ppi;            /* Scale by pixels-per-inch */
static GtkWidget *scaling_image;          /* Scale to the image */
static GtkWidget *output_gray;            /* Output type toggle, black */
static GtkWidget *output_color;           /* Output type toggle, color */
static GtkWidget *output_monochrome;
static GtkWidget *image_line_art;
static GtkWidget *image_solid_tone;
static GtkWidget *image_continuous_tone;
static GtkWidget *setup_dialog;         /* Setup dialog window */
static GtkWidget *printer_driver;       /* Printer driver widget */
static GtkWidget *printer_crawler;      /* Scrolled Window for menu */
static GtkWidget *printer_combo;	/* Combo for menu */
static gint plist_callback_id	   = -1;
static GtkWidget *ppd_file;             /* PPD file entry */
static GtkWidget *ppd_button;           /* PPD file browse button */
static GtkWidget *output_cmd;           /* Output command text entry */
static GtkWidget *ppd_browser;          /* File selection dialog for PPD files */
static GtkWidget *new_printer_dialog; /* New printer dialog window */
static GtkWidget *new_printer_entry;  /* New printer text entry */

static GtkWidget *file_browser;         /* FSD for print files */
static GtkWidget *adjust_color_button;

static GtkObject *scaling_adjustment;	/* Adjustment object for scaling */
static gboolean   suppress_scaling_adjustment = FALSE;
static gboolean   suppress_scaling_callback   = FALSE;

static gint   suppress_preview_update = 0;

static GtkDrawingArea *preview = NULL;	/* Preview drawing area widget */
static gint            mouse_x;		/* Last mouse X */
static gint            mouse_y;		/* Last mouse Y */
static gint	       old_top;		/* Previous position */
static gint	       old_left;	/* Previous position */
static gint	       buttons_pressed = 0;
static gint	       preview_active = 0;
static gint	       buttons_mask = 0;
static gint            mouse_button = -1;	/* Button being dragged with */

static gint            printable_left;	/* Left pixel column of page */
static gint            printable_top;	/* Top pixel row of page */
static gint            printable_width;	/* Width of page on screen */
static gint            printable_height;	/* Height of page on screen */
static gint            print_width;	/* Printed width of image */
static gint            print_height;	/* Printed height of image */
static gint	       left, right;	        /* Imageable area */
static gint            top, bottom;
static gint	       paper_width, paper_height;	/* Physical width */

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
static void gimp_unit_callback         (GtkWidget     *widget,
					gpointer       data);
static void gimp_orientation_callback  (GtkWidget     *widget,
					gpointer       data);
static void gimp_printandsave_callback (void);
static void gimp_print_callback        (void);
static void gimp_save_callback         (void);

static void gimp_setup_update          (void);
static void gimp_setup_open_callback   (void);
static void gimp_setup_ok_callback     (void);
static void gimp_new_printer_open_callback   (void);
static void gimp_new_printer_ok_callback     (void);
static void gimp_ppd_browse_callback   (void);
static void gimp_ppd_ok_callback       (void);
static void gimp_print_driver_callback (GtkWidget      *widget,
					gint            row,
					gint            column,
					GdkEventButton *event,
					gpointer        data);

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

static gint preview_ppi = 10;

#define THUMBNAIL_MAXW	(128)
#define THUMBNAIL_MAXH	(128)
#define Combo_get_text(combo) \
	(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry)))

gint    thumbnail_w, thumbnail_h, thumbnail_bpp;
guchar *thumbnail_data;
gint    adjusted_thumbnail_bpp;
guchar *adjusted_thumbnail_data;

static gchar *
c_strdup(const gchar *s)
{
  gchar *ret = xmalloc(strlen(s) + 1);
  strcpy(ret, s);
  return ret;
}

static char **printer_list = 0;
static int printer_count = 0;

void
gimp_build_printer_combo(void)
{
  int i;
  if (printer_list)
    {
      for (i = 0; i < printer_count; i++)
	free(printer_list[i]);
      free(printer_list);
    }
  printer_list = malloc(sizeof(char *) * plist_count);
  for (i = 0; i < plist_count; i++)
    {
      if (plist[i].active)
	{
	  printer_list[i] = malloc(strlen(plist[i].name) + 1);
	  strcpy(printer_list[i], plist[i].name);
	}
      else
	{
	  printer_list[i] = malloc(strlen(plist[i].name) + 2);
	  strcpy(printer_list[i] + 1, plist[i].name);
	  printer_list[i][0] = '*';
	}
    }
  printer_count = plist_count;
  gimp_plist_build_combo(printer_combo,
			 printer_count,
			 printer_list,
			 printer_list[plist_current],
			 gimp_plist_callback,
			 &plist_callback_id);
}

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
  GtkWidget *media_size_hbox;
  GtkWidget *ppvbox;
  GtkWidget *table;
  GtkWidget *stp_printer_table;

  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *entry;
  GtkWidget *list;       /* List of drivers */
  GtkWidget *option;
  GtkWidget *combo;      /* Combo box */
  GtkWidget *box;
  GSList    *group;
  GSList    *image_type_group;
  gint       i;
  gchar      s[100];

  stp_printer_t the_printer = stp_get_printer_by_index (0);
  gchar *plug_in_name;

  /*
   * Fetch a thumbnail of the image we're to print from the Gimp...
   */

  thumbnail_w = THUMBNAIL_MAXW;
  thumbnail_h = THUMBNAIL_MAXH;
  thumbnail_data = gimp_image_get_thumbnail_data (image_ID, &thumbnail_w,
                                                  &thumbnail_h, &thumbnail_bpp);

  /*
   * thumbnail_w and thumbnail_h have now been adjusted to the actual
   * thumbnail dimensions.  Now initialize a color-adjusted version of
   * the thumbnail...
   */

  adjusted_thumbnail_data = g_malloc (3 * thumbnail_w * thumbnail_h);

  /*
   * Create the main dialog
   */

  plug_in_name = g_strdup_printf (_("%s -- Print v%s"),
                                  image_filename, PLUG_IN_VERSION);

  print_dialog = dialog =
    gimp_dialog_new (plug_in_name, "print",
                     gimp_standard_help_func, "filters/print.html",
                     GTK_WIN_POS_MOUSE,
                     FALSE, TRUE, FALSE,

                     _("Print and\nSave Settings"), gimp_printandsave_callback,
                     NULL, NULL, NULL, FALSE, FALSE,
                     _("Save\nSettings"), gimp_save_callback,
                     NULL, NULL, NULL, FALSE, FALSE,
                     _("Print"), gimp_print_callback,
                     NULL, NULL, NULL, FALSE, FALSE,
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

  ppvbox = gtk_vbox_new (FALSE, 7);
  gtk_box_pack_end (GTK_BOX (hbox), GTK_WIDGET (ppvbox), FALSE, FALSE, 0);
  gtk_widget_show (ppvbox);

  /*
   * Drawing area for page preview...
   */

  preview = (GtkDrawingArea *) gtk_drawing_area_new ();
  gtk_drawing_area_size (preview, PREVIEW_SIZE_HORIZ + 1,
			 PREVIEW_SIZE_VERT + 1);
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

  frame = gtk_frame_new (_("Position"));
  gtk_box_pack_end (GTK_BOX (ppvbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (4, 6, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  recenter_button = button = gtk_button_new_with_label (_("Center Image"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 16, 8);
  gtk_signal_connect(GTK_OBJECT (button), "clicked",
                     GTK_SIGNAL_FUNC (gimp_position_callback),
		     NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             NULL, 0, 0,
                             button, 2, TRUE);

  box = gtk_vbox_new (FALSE, 2);
  gimp_table_attach_aligned (GTK_TABLE (table), 2, 0,
                             _("Units:"), 1.0, 0.15,
                             box, 1, TRUE);

  unit_inch = button = gtk_radio_button_new_with_label (NULL, _("Inch"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  if (stp_get_unit(vars) == 0)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
                      GTK_SIGNAL_FUNC (gimp_unit_callback),
                      (gpointer) 0);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
  unit_cm = button = gtk_radio_button_new_with_label (group, _("cm"));
  if (stp_get_unit(vars) == 1)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
                      GTK_SIGNAL_FUNC (gimp_unit_callback),
                      (gpointer) 1);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  left_entry = entry = gtk_entry_new ();
  g_snprintf (s, sizeof (s), "%.3f", fabs (stp_get_left(vars)));
  gtk_entry_set_text (GTK_ENTRY (entry), s);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
                      GTK_SIGNAL_FUNC (gimp_position_callback),
		      NULL);
  gtk_widget_set_usize (entry, 60, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Left:"), 1.0, 0.5,
                             entry, 1, TRUE);

  top_entry = entry = gtk_entry_new ();
  g_snprintf (s, sizeof (s), "%.3f", fabs (stp_get_top(vars)));
  gtk_entry_set_text (GTK_ENTRY (entry), s);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
                      GTK_SIGNAL_FUNC (gimp_position_callback),
		      NULL);
  gtk_widget_set_usize (entry, 60, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 2, 1,
                             _("Top:"), 1.0, 0.5,
                             entry, 1, TRUE);

  right_entry = entry = gtk_entry_new ();
  g_snprintf (s, sizeof (s), "%.3f", fabs (stp_get_left(vars)));
  gtk_entry_set_text (GTK_ENTRY (entry), s);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
                      GTK_SIGNAL_FUNC (gimp_position_callback),
		      NULL);
  gtk_widget_set_usize (entry, 60, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("Right:"), 1.0, 0.5,
                             entry, 1, TRUE);


  right_border_entry = entry = gtk_entry_new ();
  g_snprintf (s, sizeof (s), "%.3f", fabs (stp_get_left(vars)));
  gtk_entry_set_text (GTK_ENTRY (entry), s);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
                      GTK_SIGNAL_FUNC (gimp_position_callback),
		      NULL);
  gtk_widget_set_usize (entry, 60, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
                             _("Right Border:"), 1.0, 0.5,
                             entry, 1, TRUE);

  bottom_entry = entry = gtk_entry_new ();
  g_snprintf (s, sizeof (s), "%.3f", fabs (stp_get_left(vars)));
  gtk_entry_set_text (GTK_ENTRY (entry), s);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
                      GTK_SIGNAL_FUNC (gimp_position_callback),
		      NULL);
  gtk_widget_set_usize (entry, 60, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 2, 2,
                             _("Bottom:"), 1.0, 0.5,
                             entry, 1, TRUE);


  bottom_border_entry = entry = gtk_entry_new ();
  g_snprintf (s, sizeof (s), "%.3f", fabs (stp_get_left(vars)));
  gtk_entry_set_text (GTK_ENTRY (entry), s);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
                      GTK_SIGNAL_FUNC (gimp_position_callback),
		      NULL);
  gtk_widget_set_usize (entry, 60, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 2, 3,
                             _("Bottom Border:"), 1.0, 0.5,
                             entry, 1, TRUE);

  /*
   * Orientation option menu...
   */

  orientation_menu = option
         = gimp_option_menu_new (FALSE,
                                 _("Auto"), gimp_orientation_callback,
                                 (gpointer) ORIENT_AUTO, NULL, NULL,
				 stp_get_orientation(vars) == ORIENT_AUTO,
                                 _("Portrait"), gimp_orientation_callback,
                                 (gpointer) ORIENT_PORTRAIT, NULL, NULL,
				 stp_get_orientation(vars) == ORIENT_PORTRAIT,
                                 _("Landscape"), gimp_orientation_callback,
                                 (gpointer) ORIENT_LANDSCAPE, NULL, NULL,
				 stp_get_orientation(vars) == ORIENT_LANDSCAPE,
                                 _("Upside down"), gimp_orientation_callback,
                                 (gpointer) ORIENT_UPSIDEDOWN, NULL, NULL,
				 stp_get_orientation(vars) == ORIENT_UPSIDEDOWN,
                                 _("Seascape"), gimp_orientation_callback,
                                 (gpointer) ORIENT_SEASCAPE, NULL, NULL,
				 stp_get_orientation(vars) == ORIENT_SEASCAPE,
                                 NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 5,
                             _("Orientation:"), 1.0, 0.5,
                             option, 4, TRUE);

  /*
   *  Printer settings frame...
   */

  frame = gtk_frame_new (_("Printer Settings"));
  gtk_box_pack_start (GTK_BOX (ppvbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  table = stp_printer_table = gtk_table_new (9, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  box = gtk_hbox_new (FALSE, 4);
  label = gtk_label_new (_("Printer:"));
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  gtk_table_attach_defaults(GTK_TABLE(stp_printer_table), box, 0, 2, 0, 1);
  gtk_widget_show(box);

  /*
   * Printer option menu...
   */

  combo = printer_combo = gtk_combo_new ();
  gtk_box_pack_start (GTK_BOX (box), combo, FALSE, FALSE, 0);
  gtk_widget_show(combo);

  button = gtk_button_new_with_label (_("Setup..."));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (gimp_setup_open_callback), NULL);
  gtk_widget_show (button);

  /*
   * Define new printer dialog
   */

  button = gtk_button_new_with_label (_("New..."));
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (gimp_new_printer_open_callback), NULL);
  gtk_widget_show (button);

  /*
   * Media size combo box...
   */

  media_size_combo = combo = gtk_combo_new();
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Media Size:"), 1.0, 0.5,
                             combo, 1, TRUE);

  media_size_hbox = gtk_hbox_new(FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (media_size_hbox), 0);
  gtk_widget_show (media_size_hbox);

  label = gtk_label_new (_("Width:"));
  gtk_box_pack_start (GTK_BOX (media_size_hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  custom_size_width = entry = gtk_entry_new();
  gtk_widget_set_usize (entry, 60, 0);
  g_snprintf(s, sizeof(s), "%d", stp_get_page_width(vars));
  gtk_entry_set_text(GTK_ENTRY(entry), s);
  gtk_signal_connect(GTK_OBJECT(entry), "activate",
		     GTK_SIGNAL_FUNC(gimp_media_size_callback), NULL);
  gtk_box_pack_start(GTK_BOX(media_size_hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show(entry);

  label = gtk_label_new (_("Height:"));
  gtk_box_pack_start (GTK_BOX (media_size_hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  custom_size_height = entry = gtk_entry_new();
  gtk_widget_set_usize (entry, 60, 0);
  g_snprintf(s, sizeof(s), "%d", stp_get_page_height(vars));
  gtk_entry_set_text(GTK_ENTRY(entry), s);
  gtk_signal_connect(GTK_OBJECT(entry), "activate",
		     GTK_SIGNAL_FUNC(gimp_media_size_callback), NULL);
  gtk_box_pack_start(GTK_BOX(media_size_hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show(entry);
  gtk_table_attach_defaults(GTK_TABLE(table), media_size_hbox, 0, 2, 2, 3);

  /*
   * Media type combo box...
   */

  media_type_combo = combo = gtk_combo_new ();
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
			     _("Media Type:"), 1.0, 0.5, combo, 1, TRUE);

  /*
   * Media source combo box...
   */

  media_source_combo = combo = gtk_combo_new ();
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 4,
                             _("Media Source:"), 1.0, 0.5, combo, 1, TRUE);

  /*
   * Ink type combo box...
   */

  ink_type_combo = combo = gtk_combo_new ();
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 5,
                             _("Ink Type:"), 1.0, 0.5, combo, 1, TRUE);

  /*
   * Resolution combo box...
   */

  resolution_combo = combo = gtk_combo_new ();
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 6,
                             _("Resolution:"), 1.0, 0.5, combo, 1, TRUE);

  /*
   * Scaling...
   */
  frame = gtk_frame_new (_("Scaling"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  (stp_printer_get_printfuncs(current_printer)->media_size)
    (current_printer, vars, &paper_width, &paper_height);

  (stp_printer_get_printfuncs(current_printer)->imageable_area)
    (current_printer, vars, &left, &right, &bottom, &top);

  /* Rationalise things a bit by measuring everything from the top left */
  top = paper_height - top;
  bottom = paper_height - bottom;

  printable_width  = right - left;
  printable_height = bottom - top;

  if (stp_get_scaling(vars) < 0.0)
    {
      const stp_vars_t lower = stp_minimum_settings();
      gdouble max_ppi_scaling;
      gdouble min_ppi_scaling, min_ppi_scaling1, min_ppi_scaling2;
      min_ppi_scaling1 = 72.0 * (gdouble) image_width /
	(gdouble) printable_width;
      min_ppi_scaling2 = 72.0 * (gdouble) image_height /
	(gdouble) printable_height;
      if (min_ppi_scaling1 > min_ppi_scaling2)
	min_ppi_scaling = min_ppi_scaling1;
      else
	min_ppi_scaling = min_ppi_scaling2;
      max_ppi_scaling = min_ppi_scaling * 100 / stp_get_scaling(lower);
      scaling_adjustment =
        gimp_scale_entry_new (GTK_TABLE (table), 0, 0, _("Scaling:"), 200, 0,
                              -stp_get_scaling(vars),
			      min_ppi_scaling, max_ppi_scaling,
			      1.0, 10.0, 1, TRUE, 0, 0, NULL, NULL);
    }
  else
    {
      const stp_vars_t lower = stp_minimum_settings();
      scaling_adjustment =
        gimp_scale_entry_new (GTK_TABLE (table), 0, 0, _("Scaling:"), 200, 75,
                              stp_get_scaling(vars),
			      stp_get_scaling(lower), 100.0,
			      1.0, 10.0, 1, TRUE, 0, 0, NULL, NULL);
    }
  gtk_signal_connect (GTK_OBJECT (scaling_adjustment), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_scaling_update),
                      NULL);

  box = gtk_hbox_new (FALSE, 7);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1, NULL, 0, 0, box, 1,
			     FALSE);

  scaling_percent = button =
    gtk_radio_button_new_with_label (NULL, _("Percent"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  if (stp_get_scaling(vars) > 0.0)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
                      GTK_SIGNAL_FUNC (gimp_scaling_callback), NULL);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  scaling_ppi = button = gtk_radio_button_new_with_label (group, _("PPI"));
  if (stp_get_scaling(vars) < 0.0)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
                      GTK_SIGNAL_FUNC (gimp_scaling_callback), NULL);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  scaling_image = button = gtk_button_new_with_label (_("Set Image Scale"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (gimp_scaling_callback), NULL);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  /*
   * Use a dummy label as a spacer
   */
  label = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (box), label, TRUE, FALSE, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Width:"));
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  width_entry = entry = gtk_entry_new ();
  g_snprintf (s, sizeof (s), "%.3f", fabs (stp_get_left(vars)));
  gtk_entry_set_text (GTK_ENTRY (entry), s);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
                      GTK_SIGNAL_FUNC (gimp_position_callback), NULL);
  gtk_widget_set_usize (entry, 60, 0);
  gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  label = gtk_label_new(_("Height:"));
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  height_entry = entry = gtk_entry_new ();
  g_snprintf (s, sizeof (s), "%.3f", fabs (stp_get_left(vars)));
  gtk_entry_set_text (GTK_ENTRY (entry), s);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
                      GTK_SIGNAL_FUNC (gimp_position_callback), NULL);
  gtk_widget_set_usize (entry, 60, 0);
  gtk_box_pack_end (GTK_BOX (box), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  /*
   * Image type
   */
  frame = gtk_frame_new (_("Image Settings"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  box = gtk_hbox_new (FALSE, 7);
  gtk_container_set_border_width (GTK_CONTAINER (box), 2);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (box);

  label = gtk_label_new (_("Image Type:"));
  gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_set_spacing(GTK_BOX(vbox), -4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
  gtk_container_add (GTK_CONTAINER (box), vbox);
  gtk_widget_show (vbox);

  image_line_art = button =
    gtk_radio_button_new_with_label (NULL, _("Line Art"));
  image_type_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  if (stp_get_image_type(vars) == IMAGE_LINE_ART)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (gimp_image_type_callback),
		      (gpointer) IMAGE_LINE_ART);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  image_solid_tone= button =
    gtk_radio_button_new_with_label (image_type_group, _("Solid Colors"));
  image_type_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  if (stp_get_image_type(vars) == IMAGE_SOLID_TONE)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (gimp_image_type_callback),
		      (gpointer) IMAGE_SOLID_TONE);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  image_continuous_tone = button =
    gtk_radio_button_new_with_label (image_type_group, _("Photograph"));
  image_type_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  if (stp_get_image_type(vars) == IMAGE_CONTINUOUS)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (gimp_image_type_callback),
		      (gpointer) IMAGE_CONTINUOUS);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*
   *  Color adjust button
   */
  gimp_create_color_adjust_window ();

  adjust_color_button = button =
    gtk_button_new_with_label (_("Adjust Color..."));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_show),
			     GTK_OBJECT (gimp_color_adjust_dialog));
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*
   * Output type toggles...
   */

  label = gtk_label_new (_("Output Type:"));
  gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_box_set_spacing(GTK_BOX(vbox), -4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
  gtk_box_pack_end (GTK_BOX (box), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  output_color = button = gtk_radio_button_new_with_label (NULL, _("Color"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  if (stp_get_output_type(vars) == OUTPUT_COLOR)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
                      GTK_SIGNAL_FUNC (gimp_output_type_callback),
                      (gpointer) OUTPUT_COLOR);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  output_gray = button = gtk_radio_button_new_with_label (group, _("Grayscale"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  if (stp_get_output_type(vars) == OUTPUT_GRAY)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
                      GTK_SIGNAL_FUNC (gimp_output_type_callback),
                      (gpointer) OUTPUT_GRAY);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  output_monochrome = button =
    gtk_radio_button_new_with_label (group, _("Black and White"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  if (stp_get_output_type(vars) == OUTPUT_MONOCHROME)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
                      GTK_SIGNAL_FUNC (gimp_output_type_callback),
                      (gpointer) OUTPUT_MONOCHROME);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*
   * Setup dialog window...
   */

  setup_dialog = dialog =
    gimp_dialog_new (_("Setup"), "print",
                     gimp_standard_help_func, "filters/print.html",
                     GTK_WIN_POS_MOUSE, FALSE, TRUE, FALSE,

                     _("OK"), gimp_setup_ok_callback,
		     NULL, NULL, NULL, TRUE, FALSE,
                     _("Cancel"), gtk_widget_hide,
                     NULL, 1, NULL, FALSE, TRUE, NULL);

  /*
   * Top-level table for dialog...
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
   * Printer driver option menu...
   */

  label = gtk_label_new (_("Printer Model:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  printer_crawler = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (printer_crawler),
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  printer_driver = list = gtk_clist_new(1);
  gtk_clist_set_selection_mode(GTK_CLIST(list), GTK_SELECTION_SINGLE);
  gtk_signal_connect(GTK_OBJECT(list), "select_row",
		     (GtkSignalFunc)gimp_print_driver_callback,
		     NULL);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW (printer_crawler),
					list);
  gtk_widget_set_usize(printer_crawler, 200, 0);
  gtk_widget_show (list);
  for (i = 0; i < stp_known_printers(); i ++)
    {
      char *tmp;
      the_printer = stp_get_printer_by_index(i);
      if (!strcmp(stp_printer_get_long_name(the_printer), ""))
	continue;
      tmp = c_strdup(gettext(stp_printer_get_long_name(the_printer)));
      gtk_clist_insert(GTK_CLIST(list), i, &tmp);
      gtk_clist_set_row_data(GTK_CLIST(list), i, (gpointer)i);
    }
  gtk_table_attach (GTK_TABLE (table), printer_crawler, 1, 3, 0, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (printer_crawler);

  /*
   * PPD file...
   */

  label = gtk_label_new (_("PPD File:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  box = gtk_hbox_new (FALSE, 8);
  gtk_table_attach (GTK_TABLE (table), box, 1, 2, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (box);

  ppd_file = entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (box), entry, TRUE, TRUE, 0);
  gtk_widget_show (entry);

  ppd_button = button = gtk_button_new_with_label (_("Browse"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (gimp_ppd_browse_callback), NULL);
  gtk_widget_show (button);

  /*
   * Print command...
   */

  label = gtk_label_new (_("Command:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  output_cmd = entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 3, 4,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (entry);

  /*
   * Output file selection dialog...
   */

  file_browser = gtk_file_selection_new (_("Print To File?"));
  gtk_signal_connect(GTK_OBJECT (GTK_FILE_SELECTION (file_browser)->ok_button),
		     "clicked", GTK_SIGNAL_FUNC (gimp_file_ok_callback), NULL);
  gtk_signal_connect(GTK_OBJECT (GTK_FILE_SELECTION (file_browser)->cancel_button),
		     "clicked", GTK_SIGNAL_FUNC (gimp_file_cancel_callback),
		     NULL);

  /*
   * PPD file selection dialog...
   */

  ppd_browser = gtk_file_selection_new (_("PPD File?"));
  gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (ppd_browser));
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (ppd_browser)->ok_button),
                      "clicked", GTK_SIGNAL_FUNC (gimp_ppd_ok_callback), NULL);
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (ppd_browser)->cancel_button),
                             "clicked", GTK_SIGNAL_FUNC (gtk_widget_hide),
                             GTK_OBJECT (ppd_browser));

  new_printer_dialog = dialog =
    gimp_dialog_new (_("Define New Printer"), "print",
                     gimp_standard_help_func, "filters/print.html",
                     GTK_WIN_POS_MOUSE, FALSE, TRUE, FALSE,

                     _("OK"), gimp_new_printer_ok_callback,
		     NULL, NULL, NULL, TRUE, FALSE,
                     _("Cancel"), gtk_widget_hide,
                     NULL, 1, NULL, FALSE, TRUE, NULL);

  table = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 8);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), table,
                      FALSE, FALSE, 0);
  gtk_widget_show (table);

  new_printer_entry = entry = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(entry), 127);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
                      GTK_SIGNAL_FUNC (gimp_new_printer_ok_callback), NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Printer Name:"), 1.0, 0.5, entry, 1, TRUE);

  /*
   * Show the main dialog and wait for the user to do something...
   */

  gimp_build_printer_combo();
  gimp_plist_callback (NULL, (gpointer) plist_current);
  gimp_update_adjusted_thumbnail ();

  gtk_widget_show (print_dialog);
}

/*
 *  gimp_scaling_update() - Update the scaling scale using the slider.
 */
static void
gimp_scaling_update (GtkAdjustment *adjustment)
{
  buttons_pressed = preview_active = 0;
  if (stp_get_scaling(vars) != adjustment->value)
    {
      if (GTK_TOGGLE_BUTTON (scaling_ppi)->active)
        stp_set_scaling(vars, -adjustment->value);
      else
        stp_set_scaling(vars, adjustment->value);
      stp_set_scaling(plist[plist_current].v, stp_get_scaling(vars));
    }

  suppress_scaling_adjustment = TRUE;
  gimp_preview_update ();
  suppress_scaling_adjustment = FALSE;
}

/*
 *  gimp_scaling_callback() - Update the scaling scale using radio buttons.
 */
static void
gimp_scaling_callback (GtkWidget *widget)
{
  const stp_vars_t lower = stp_minimum_settings ();
  gdouble max_ppi_scaling;
  gdouble min_ppi_scaling, min_ppi_scaling1, min_ppi_scaling2;
  gdouble current_scale;
  buttons_pressed = preview_active = 0;

  if (suppress_scaling_callback)
    return;

  min_ppi_scaling1 = 72.0 * (gdouble) image_width /
    (gdouble) printable_width;
  min_ppi_scaling2 = 72.0 * (gdouble) image_height /
    (gdouble) printable_height;
  if (min_ppi_scaling1 > min_ppi_scaling2)
    min_ppi_scaling = min_ppi_scaling1;
  else
    min_ppi_scaling = min_ppi_scaling2;
  max_ppi_scaling = min_ppi_scaling * 100 / stp_get_scaling(lower);
  if (widget == scaling_ppi)
    {
      if (!(GTK_TOGGLE_BUTTON (scaling_ppi)->active))
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
      stp_set_scaling(vars, 0.0);
      stp_set_scaling(plist[plist_current].v, stp_get_scaling(vars));
    }
  else if (widget == scaling_percent)
    {
      gdouble new_percent;
      if (!(GTK_TOGGLE_BUTTON (scaling_percent)->active))
	return;
      current_scale = GTK_ADJUSTMENT (scaling_adjustment)->value;
      GTK_ADJUSTMENT (scaling_adjustment)->lower = stp_get_scaling(lower);
      GTK_ADJUSTMENT (scaling_adjustment)->upper = 100.0;

      new_percent = 100 * min_ppi_scaling / current_scale;
      if (new_percent > 100)
	new_percent = 100;
      if (new_percent < stp_get_scaling(lower))
	new_percent = stp_get_scaling(lower);
      GTK_ADJUSTMENT (scaling_adjustment)->value = new_percent;
      stp_set_scaling(vars, 0.0);
      stp_set_scaling(plist[plist_current].v, stp_get_scaling(vars));
    }
  else if (widget == scaling_image)
    {
      gdouble xres, yres;

      gimp_image_get_resolution (image_ID, &xres, &yres);

      GTK_ADJUSTMENT (scaling_adjustment)->lower = min_ppi_scaling;
      GTK_ADJUSTMENT (scaling_adjustment)->upper = max_ppi_scaling;
      if (yres < min_ppi_scaling)
	yres = min_ppi_scaling;
      if (yres > max_ppi_scaling)
	yres = max_ppi_scaling;

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scaling_ppi), TRUE);
      GTK_ADJUSTMENT (scaling_adjustment)->value = yres;
      stp_set_scaling(vars, 0.0);
      stp_set_scaling(plist[plist_current].v, stp_get_scaling(vars));
    }

  gtk_adjustment_changed (GTK_ADJUSTMENT (scaling_adjustment));
  gtk_adjustment_value_changed (GTK_ADJUSTMENT (scaling_adjustment));
}

/****************************************************************************
 *
 * gimp_plist_build_combo
 *
 ****************************************************************************/
void
gimp_plist_build_combo (GtkWidget      *combo,       /* I - Combo widget */
			gint            num_items,   /* I - Number of items */
			gchar    **items,       /* I - Menu items */
			const gchar     *cur_item,    /* I - Current item */
			GtkSignalFunc   callback,    /* I - Callback */
			gint           *callback_id) /* IO - Callback ID (init to -1) */
{
  gint      i; /* Looping var */
  GList    *list = 0;
  GtkEntry *entry = GTK_ENTRY (GTK_COMBO (combo)->entry);
  char *ncur_item;

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
    list = g_list_append (list, c_strdup(gettext (items[i])));

  gtk_combo_set_popdown_strings (GTK_COMBO (combo), list);

  *callback_id = gtk_signal_connect (GTK_OBJECT (entry), "changed",
				     callback,
				     NULL);
  ncur_item = c_strdup(cur_item);

  gtk_entry_set_text (entry, ncur_item);

  for (i = 0; i < num_items; i ++)
    if (strcmp(items[i], ncur_item) == 0)
      break;

  if (i == num_items)
    gtk_entry_set_text (entry, c_strdup(gettext (items[0])));

  gtk_combo_set_value_in_list (GTK_COMBO (combo), TRUE, FALSE);
  gtk_widget_set_sensitive (combo, TRUE);
  gtk_widget_show (combo);
}

/*
 *  gimp_do_misc_updates() - Build an option menu for the given parameters...
 */
static void
gimp_do_misc_updates (void)
{
  const stp_vars_t lower = stp_minimum_settings ();

  stp_set_scaling(vars, stp_get_scaling(plist[plist_current].v));
  stp_set_orientation(vars, stp_get_orientation(plist[plist_current].v));
  stp_set_left(vars, stp_get_left(plist[plist_current].v));
  stp_set_top(vars, stp_get_top(plist[plist_current].v));
  stp_set_unit(vars, stp_get_unit(plist[plist_current].v));

  suppress_preview_update++;
  gimp_preview_update ();

  if (stp_get_scaling(plist[plist_current].v) < 0)
    {
      gdouble tmp = -stp_get_scaling(plist[plist_current].v);
      gdouble max_ppi_scaling;
      gdouble min_ppi_scaling, min_ppi_scaling1, min_ppi_scaling2;

      min_ppi_scaling1 = 72.0 * (gdouble) image_width /
	(gdouble) printable_width;
      min_ppi_scaling2 = 72.0 * (gdouble) image_height /
	(gdouble) printable_height;

      if (min_ppi_scaling1 > min_ppi_scaling2)
	min_ppi_scaling = min_ppi_scaling1;
      else
	min_ppi_scaling = min_ppi_scaling2;

      max_ppi_scaling = min_ppi_scaling * 100 / stp_get_scaling(lower);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scaling_ppi), TRUE);
      GTK_ADJUSTMENT (scaling_adjustment)->lower = min_ppi_scaling;
      GTK_ADJUSTMENT (scaling_adjustment)->upper = max_ppi_scaling;
      GTK_ADJUSTMENT (scaling_adjustment)->value = tmp;
      gtk_adjustment_changed (GTK_ADJUSTMENT (scaling_adjustment));
      gtk_adjustment_value_changed (GTK_ADJUSTMENT (scaling_adjustment));
      stp_set_scaling(plist[plist_current].v, stp_get_scaling(vars));
    }
  else
    {
      gdouble tmp = stp_get_scaling(plist[plist_current].v);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scaling_percent), TRUE);
      GTK_ADJUSTMENT (scaling_adjustment)->lower = stp_get_scaling(lower);
      GTK_ADJUSTMENT (scaling_adjustment)->upper = 100.0;
      GTK_ADJUSTMENT (scaling_adjustment)->value = tmp;
      gtk_signal_emit_by_name (scaling_adjustment, "changed");
      gtk_signal_emit_by_name (scaling_adjustment, "value_changed");
    }

  switch (stp_get_output_type(plist[plist_current].v))
    {
    case OUTPUT_GRAY:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(output_gray), TRUE);
      break;
    case OUTPUT_COLOR:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(output_color), TRUE);
      break;
    case OUTPUT_MONOCHROME:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(output_monochrome), TRUE);
      break;
    }

  gimp_do_color_updates ();

  gtk_option_menu_set_history (GTK_OPTION_MENU (orientation_menu),
			       stp_get_orientation(vars) + 1);

  if (stp_get_unit(plist[plist_current].v) == 0)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (unit_inch), TRUE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (unit_cm), TRUE);

  switch (stp_get_image_type(plist[plist_current].v))
  {
    case IMAGE_LINE_ART:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (image_line_art), TRUE);
      break;
    case IMAGE_SOLID_TONE:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (image_solid_tone), TRUE);
      break;
    case IMAGE_CONTINUOUS:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (image_continuous_tone),
				   TRUE);
      break;
    default:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (image_continuous_tone),
				   TRUE);
      stp_set_image_type(plist[plist_current].v, IMAGE_CONTINUOUS);
      break;
    }

  suppress_preview_update--;
  gimp_preview_update ();
}

/*
 * gimp_position_callback() - callback for position entry widgets
 */
static void
gimp_position_callback (GtkWidget *widget)
{
  buttons_pressed = preview_active = 0;
  suppress_preview_update++;
  if (widget == recenter_button)
    {
      stp_set_left(vars, -1);
      stp_set_top(vars, -1);
    }
  else
    {
      gdouble new_value = atof (gtk_entry_get_text (GTK_ENTRY (widget)));
      gdouble unit_scaler = 1.0;
      gboolean was_percent = 0;

      if (stp_get_unit(vars))
	unit_scaler /= 2.54;
      new_value *= unit_scaler;

      if (widget == top_entry)
	stp_set_top(vars, ((new_value + (1.0 / 144.0)) * 72) - top);
      else if (widget == bottom_entry)
	stp_set_top(vars,
		    ((new_value + (1.0 / 144.0)) * 72) - (top + print_height));
      else if (widget == bottom_border_entry)
	stp_set_top(vars,
		    paper_height - print_height - top - (new_value * 72));
      else if (widget == left_entry)
	stp_set_left(vars, ((new_value + (1.0 / 144.0)) * 72) - left);
      else if (widget == right_entry)
	stp_set_left(vars,
		     ((new_value + (1.0 / 144.0)) * 72) - (left +print_width));
      else if (widget == right_border_entry)
	stp_set_left(vars,
		     paper_width - print_width - left - (new_value * 72));
      else if (widget == width_entry)
	{
	  if (stp_get_scaling(vars) >= 0)
	    {
	      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scaling_ppi),
					    TRUE);
	      gimp_scaling_callback (scaling_ppi);
	      was_percent = 1;
	    }
	  GTK_ADJUSTMENT (scaling_adjustment)->value =
	    ((gdouble) image_width) / new_value;
	  gtk_adjustment_value_changed (GTK_ADJUSTMENT (scaling_adjustment));
	  if (was_percent)
	    {
	      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (scaling_percent),
					   TRUE);
	      gtk_adjustment_value_changed(GTK_ADJUSTMENT(scaling_adjustment));
	    }
	}
      else if (widget == height_entry)
	{
	  if (stp_get_scaling(vars) >= 0)
	    {
	      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scaling_ppi),
					    TRUE);
	      gimp_scaling_callback (scaling_ppi);
	      was_percent = 1;
	    }
	  GTK_ADJUSTMENT (scaling_adjustment)->value =
	    ((gdouble) image_height) / new_value;
	  gtk_adjustment_value_changed (GTK_ADJUSTMENT (scaling_adjustment));
	  if (was_percent)
	    {
	      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (scaling_percent),
					   TRUE);
	      gtk_adjustment_value_changed(GTK_ADJUSTMENT(scaling_adjustment));
	    }
	}
      if (stp_get_left(vars) < 0)
	stp_set_left(vars, 0);
      if (stp_get_top(vars) < 0)
	stp_set_top(vars, 0);
    }

  stp_set_left(plist[plist_current].v, stp_get_left(vars));
  stp_set_top(plist[plist_current].v, stp_get_top(vars));
  suppress_preview_update--;
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
  gp_plist_t *p;
  gint		num_media_sizes;
  gchar		**media_sizes;
  gint		num_media_types;	/* Number of media types */
  gchar		**media_types;		/* Media type strings */
  gint		num_media_sources;	/* Number of media sources */
  gchar		**media_sources;        /* Media source strings */
  gint		num_ink_types;		/* Number of ink types */
  gchar		**ink_types;		/* Ink type strings */
  gint		num_resolutions;	/* Number of resolutions */
  gchar		**resolutions;		/* Resolution strings */
  buttons_pressed = preview_active = 0;

  if (widget)
    {
      const gchar *result = Combo_get_text(printer_combo);
      for (i = 0; i < plist_count; i++)
	{
	  if (!strcmp(result, printer_list[i]))
	    {
	      plist_current = i;
	      break;
	    }
	}
    }
  else
    plist_current = (gint) data;
  p             = plist + plist_current;

  if (strcmp(stp_get_driver(p->v), ""))
    {
      stp_set_driver(vars, stp_get_driver(p->v));

      current_printer = stp_get_printer_by_driver(stp_get_driver(vars));
    }

  stp_set_ppd_file(vars, stp_get_ppd_file(p->v));
  stp_set_media_size(vars, stp_get_media_size(p->v));
  stp_set_media_type(vars, stp_get_media_type(p->v));
  stp_set_media_source(vars, stp_get_media_source(p->v));
  stp_set_ink_type(vars, stp_get_ink_type(p->v));
  stp_set_dither_algorithm(vars, stp_get_dither_algorithm(p->v));
  stp_set_resolution(vars, stp_get_resolution(p->v));
  stp_set_output_to(vars, stp_get_output_to(p->v));

  suppress_preview_update++;
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (dither_algo_combo)->entry),
                      stp_get_dither_algorithm(vars));

  gimp_setup_update ();

  gimp_do_misc_updates ();

  /*
   * Now get option parameters...
   */

  media_sizes = (*(stp_printer_get_printfuncs(current_printer)->parameters))
    (current_printer, stp_get_ppd_file(p->v), "PageSize", &num_media_sizes);
  if (stp_get_media_size(vars)[0] == '\0')
    stp_set_media_size(vars, media_sizes[0]);
  gimp_plist_build_combo (media_size_combo,
			  num_media_sizes,
			  media_sizes,
			  stp_get_media_size(p->v),
			  gimp_media_size_callback,
			  &media_size_callback_id);

  if (num_media_sizes > 0)
    {
      for (i = 0; i < num_media_sizes; i ++)
	free (media_sizes[i]);
      free (media_sizes);
    }

  media_types = (*(stp_printer_get_printfuncs(current_printer)->parameters))
    (current_printer, stp_get_ppd_file(p->v), "MediaType", &num_media_types);
  if (stp_get_media_type(vars)[0] == '\0' && media_types != NULL)
    stp_set_media_type(vars, media_types[0]);
  else if (media_types == NULL)
    stp_set_media_type(vars, NULL);
  gimp_plist_build_combo (media_type_combo,
			  num_media_types,
			  media_types,
			  stp_get_media_type(p->v),
			  gimp_media_type_callback,
			  &media_type_callback_id);

  if (num_media_types > 0)
    {
      for (i = 0; i < num_media_types; i ++)
	free (media_types[i]);
      free (media_types);
    }

  media_sources = (*(stp_printer_get_printfuncs(current_printer)->parameters))
    (current_printer, stp_get_ppd_file(p->v), "InputSlot", &num_media_sources);
  if (stp_get_media_source(vars)[0] == '\0' && media_sources != NULL)
    stp_set_media_source(vars, media_sources[0]);
  else if (media_sources == NULL)
    stp_set_media_source(vars, NULL);
  gimp_plist_build_combo (media_source_combo,
			  num_media_sources,
			  media_sources,
			  stp_get_media_source(p->v),
			  gimp_media_source_callback,
			  &media_source_callback_id);

  if (num_media_sources > 0)
    {
      for (i = 0; i < num_media_sources; i ++)
	free (media_sources[i]);
      free (media_sources);
    }

  ink_types = (*(stp_printer_get_printfuncs(current_printer)->parameters))
    (current_printer, stp_get_ppd_file(p->v), "InkType", &num_ink_types);
  if (stp_get_ink_type(vars)[0] == '\0' && ink_types != NULL)
    stp_set_ink_type(vars, ink_types[0]);
  else if (ink_types == NULL)
    stp_set_ink_type(vars, NULL);
  gimp_plist_build_combo (ink_type_combo,
			  num_ink_types,
			  ink_types,
			  stp_get_ink_type(p->v),
			  gimp_ink_type_callback,
			  &ink_type_callback_id);

  if (num_ink_types > 0)
    {
      for (i = 0; i < num_ink_types; i ++)
	free (ink_types[i]);
      free (ink_types);
    }

  resolutions = (*(stp_printer_get_printfuncs(current_printer)->parameters))
    (current_printer, stp_get_ppd_file(p->v), "Resolution", &num_resolutions);
  if (stp_get_resolution(vars)[0] == '\0' && resolutions != NULL)
    stp_set_resolution(vars, resolutions[0]);
  else if (resolutions == NULL)
    stp_set_resolution(vars, NULL);
  gimp_plist_build_combo (resolution_combo,
			  num_resolutions,
			  resolutions,
			  stp_get_resolution(p->v),
			  gimp_resolution_callback,
			  &resolution_callback_id);

  if (num_resolutions > 0)
    {
      for (i = 0; i < num_resolutions; i ++)
	free (resolutions[i]);
      free (resolutions);
    }
  if (dither_algo_combo)
    gimp_build_dither_combo();
  suppress_preview_update--;
  gimp_preview_update();
}

/*
 *  gimp_media_size_callback() - Update the current media size...
 */
static void
gimp_media_size_callback (GtkWidget *widget,
			  gpointer   data)
{
  gchar s[32];
  buttons_pressed = preview_active = 0;
  if (widget == custom_size_width)
    {
      gint width_limit, height_limit;
      gdouble new_value = atof (gtk_entry_get_text (GTK_ENTRY (widget)));
      gdouble unit_scaler = 1.0;
      new_value *= 72;
      if (stp_get_unit(vars))
	unit_scaler /= 2.54;
      new_value *= unit_scaler;
      (stp_printer_get_printfuncs(current_printer)->limit)
	(current_printer, vars, &width_limit, &height_limit);
      if (new_value < 72)
	new_value = 72;
      else if (new_value > width_limit)
	new_value = width_limit;
      stp_set_page_width(plist[plist_current].v, new_value);
      stp_set_page_width(vars, new_value);
      stp_set_left(vars, -1);
      stp_set_left(plist[plist_current].v, stp_get_left(vars));
      new_value = new_value / 72.0;
      if (stp_get_unit(vars))
	new_value *= 2.54;
      g_snprintf(s, sizeof(s), "%.2f", new_value);
      gtk_entry_set_text(GTK_ENTRY(custom_size_width), s);
      gimp_preview_update ();
    }
  else if (widget == custom_size_height)
    {
      gint width_limit, height_limit;
      gdouble new_value = atof (gtk_entry_get_text (GTK_ENTRY (widget)));
      gdouble unit_scaler = 1.0;
      new_value *= 72;
      if (stp_get_unit(vars))
	unit_scaler /= 2.54;
      new_value *= unit_scaler;
      (stp_printer_get_printfuncs(current_printer)->limit)
	(current_printer, vars, &width_limit, &height_limit);
      if (new_value < 144)
	new_value = 144;
      else if (new_value > height_limit)
	new_value = height_limit;
      stp_set_page_height(plist[plist_current].v, new_value);
      stp_set_page_height(vars, new_value);
      stp_set_top(vars, -1);
      stp_set_top(plist[plist_current].v, stp_get_top(vars));
      new_value = new_value / 72.0;
      if (stp_get_unit(vars))
	new_value *= 2.54;
      g_snprintf(s, sizeof(s), "%.2f", new_value);
      gtk_entry_set_text(GTK_ENTRY(custom_size_height), s);
      gimp_preview_update ();
    }
  else
    {
      const gchar *new_media_size = Combo_get_text (media_size_combo);
      const stp_papersize_t pap = stp_get_papersize_by_name(new_media_size);
      if (pap)
	{
	  gint default_width, default_height;
	  gdouble size;
	  if (stp_papersize_get_width(pap) == 0)
	    {
	      (stp_printer_get_printfuncs(current_printer)->media_size)
		(current_printer, vars, &default_width, &default_height);
	      size = default_width / 72.0;
	      if (stp_get_unit(vars))
		size *= 2.54;
	      g_snprintf(s, sizeof(s), "%.2f", size);
	      gtk_entry_set_text(GTK_ENTRY(custom_size_width), s);
	      gtk_widget_set_sensitive(GTK_WIDGET(custom_size_width), TRUE);
	      gtk_entry_set_editable(GTK_ENTRY(custom_size_width), TRUE);
	      stp_set_page_width(plist[plist_current].v, default_width);
	      stp_set_page_width(vars, default_width);
	    }
	  else
	    {
	      size = stp_papersize_get_width(pap) / 72.0;
	      if (stp_get_unit(vars))
		size *= 2.54;
	      g_snprintf(s, sizeof(s), "%.2f", size);
	      gtk_entry_set_text(GTK_ENTRY(custom_size_width), s);
	      gtk_widget_set_sensitive(GTK_WIDGET(custom_size_width), FALSE);
	      gtk_entry_set_editable(GTK_ENTRY(custom_size_width), FALSE);
	      stp_set_page_width(plist[plist_current].v,
				 stp_papersize_get_width(pap));
	      stp_set_page_width(vars, stp_papersize_get_width(pap));
	    }
	  if (stp_papersize_get_height(pap) == 0)
	    {
	      (stp_printer_get_printfuncs(current_printer)->media_size)
		(current_printer, vars, &default_width, &default_height);
	      size = default_height / 72.0;
	      if (stp_get_unit(vars))
		size *= 2.54;
	      g_snprintf(s, sizeof(s), "%.2f", size);
	      gtk_entry_set_text(GTK_ENTRY(custom_size_height), s);
	      gtk_widget_set_sensitive(GTK_WIDGET(custom_size_height), TRUE);
	      gtk_entry_set_editable(GTK_ENTRY(custom_size_height), TRUE);
	      stp_set_page_height(plist[plist_current].v, default_height);
	      stp_set_page_height(vars, default_height);
	    }
	  else
	    {
	      size = stp_papersize_get_height(pap) / 72.0;
	      if (stp_get_unit(vars))
		size *= 2.54;
	      g_snprintf(s, sizeof(s), "%.2f", size);
	      gtk_entry_set_text(GTK_ENTRY(custom_size_height), s);
	      gtk_widget_set_sensitive(GTK_WIDGET(custom_size_height), FALSE);
	      gtk_entry_set_editable(GTK_ENTRY(custom_size_height), FALSE);
	      stp_set_page_height(plist[plist_current].v,
				  stp_papersize_get_height(pap));
	      stp_set_page_height(vars, stp_papersize_get_height(pap));
	    }
	}
      if (strcmp (stp_get_media_size(vars), new_media_size) != 0)
	{
	  stp_set_media_size(vars, new_media_size);
	  stp_set_media_size(plist[plist_current].v,new_media_size);
	  stp_set_left(vars, -1);
	  stp_set_top(vars, -1);
	  stp_set_left(plist[plist_current].v, stp_get_left(vars));
	  stp_set_top(plist[plist_current].v, stp_get_top(vars));
	  gimp_preview_update ();
	}
    }
}

/*
 *  gimp_media_type_callback() - Update the current media type...
 */
static void
gimp_media_type_callback (GtkWidget *widget,
			  gpointer   data)
{
  const gchar *new_media_type = Combo_get_text (media_type_combo);
  buttons_pressed = preview_active = 0;
  stp_set_media_type(vars, new_media_type);
  stp_set_media_type(plist[plist_current].v, new_media_type);
  gimp_preview_update ();
}

/*
 *  gimp_media_source_callback() - Update the current media source...
 */
static void
gimp_media_source_callback (GtkWidget *widget,
			    gpointer   data)
{
  const gchar *new_media_source = Combo_get_text (media_source_combo);
  buttons_pressed = preview_active = 0;
  stp_set_media_source(vars, new_media_source);
  stp_set_media_source(plist[plist_current].v, new_media_source);
  gimp_preview_update ();
}

/*
 *  gimp_ink_type_callback() - Update the current ink type...
 */
static void
gimp_ink_type_callback (GtkWidget *widget,
			gpointer   data)
{
  const gchar *new_ink_type = Combo_get_text (ink_type_combo);
  buttons_pressed = preview_active = 0;
  stp_set_ink_type(vars, new_ink_type);
  stp_set_ink_type(plist[plist_current].v, new_ink_type);
  gimp_preview_update ();
}

/*
 *  gimp_resolution_callback() - Update the current resolution...
 */
static void
gimp_resolution_callback (GtkWidget *widget,
			  gpointer   data)
{
  const gchar *new_resolution = Combo_get_text (resolution_combo);
  buttons_pressed = preview_active = 0;
  stp_set_resolution(vars, new_resolution);
  stp_set_resolution(plist[plist_current].v, new_resolution);
  gimp_preview_update ();
}

/*
 *  gimp_orientation_callback() - Update the current media size...
 */
static void
gimp_orientation_callback (GtkWidget *widget,
			   gpointer   data)
{
  buttons_pressed = preview_active = 0;
  if (stp_get_orientation(vars) != (gint) data)
    {
      stp_set_orientation(vars, (gint) data);
      stp_set_left(vars, -1);
      stp_set_top(vars, -1);
      stp_set_orientation(plist[plist_current].v, stp_get_orientation(vars));
      stp_set_left(plist[plist_current].v, stp_get_left(vars));
      stp_set_top(plist[plist_current].v, stp_get_top(vars));
    }
  gimp_preview_update ();
}

/*
 *  gimp_output_type_callback() - Update the current output type...
 */
static void
gimp_output_type_callback (GtkWidget *widget,
			   gpointer   data)
{
  buttons_pressed = preview_active = 0;
  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      stp_set_output_type(vars, (gint) data);
      stp_set_output_type(plist[plist_current].v, (gint) data);
      gimp_update_adjusted_thumbnail();
    }
  if (widget == output_color)
    gimp_set_color_sliders_active(TRUE);
  else
    gimp_set_color_sliders_active(FALSE);
  gimp_preview_update ();
}

/*
 *  gimp_unit_callback() - Update the current unit...
 */
static void
gimp_unit_callback (GtkWidget *widget,
			   gpointer   data)
{
  buttons_pressed = preview_active = 0;
  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      stp_set_unit(vars, (gint) data);
      stp_set_unit(plist[plist_current].v, (gint) data);
      gimp_preview_update();
    }
}

/*
 *  gimp_image_type_callback() - Update the current image type mode...
 */
static void
gimp_image_type_callback (GtkWidget *widget,
			  gpointer   data)
{
  buttons_pressed = preview_active = 0;
  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      stp_set_image_type(vars, (gint) data);
      gimp_update_adjusted_thumbnail();
      stp_set_image_type(plist[plist_current].v, (gint) data);
    }
  gimp_preview_update ();
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
      gtk_widget_destroy (gimp_color_adjust_dialog);
      gtk_widget_destroy (setup_dialog);
      gtk_widget_destroy (print_dialog);
    }
  else
    {
      gtk_widget_set_sensitive (gimp_color_adjust_dialog, FALSE);
      gtk_widget_set_sensitive (setup_dialog, FALSE);
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
      gtk_widget_destroy (gimp_color_adjust_dialog);
      gtk_widget_destroy (setup_dialog);
      gtk_widget_destroy (print_dialog);
    }
  else
    {
      gtk_widget_set_sensitive (gimp_color_adjust_dialog, FALSE);
      gtk_widget_set_sensitive (setup_dialog, FALSE);
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
  buttons_pressed = preview_active = 0;
  printrc_save ();
}

/*
 *  gimp_setup_update() - update widgets in the setup dialog
 */
static void
gimp_setup_update (void)
{
  GtkAdjustment *adjustment;
  gint idx;

  current_printer = stp_get_printer_by_driver(stp_get_driver(plist[plist_current].v));
  idx = stp_get_printer_index_by_driver(stp_get_driver(plist[plist_current].v));

  gtk_clist_select_row(GTK_CLIST(printer_driver), idx, 0);

  gtk_entry_set_text (GTK_ENTRY (ppd_file),
		      stp_get_ppd_file(plist[plist_current].v));

  if (strncmp (stp_get_driver(plist[plist_current].v),"ps", 2) == 0)
    {
      gtk_widget_show (ppd_file);
      gtk_widget_show (ppd_button);
    }
  else
    {
      gtk_widget_hide (ppd_file);
      gtk_widget_hide (ppd_button);
    }

  gtk_entry_set_text (GTK_ENTRY (output_cmd),
		      stp_get_output_to(plist[plist_current].v));

  if (plist_current == 0)
    gtk_widget_hide (output_cmd);
  else
    gtk_widget_show (output_cmd);

  adjustment =
    gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW (printer_crawler));
  gtk_adjustment_set_value(adjustment, idx * (adjustment->step_increment + 5));
}

/*
 *  gimp_setup_open_callback() -
 */
static void
gimp_setup_open_callback (void)
{
  static int first_time = 1;

  buttons_pressed = preview_active = 0;
  gimp_setup_update ();

  gtk_widget_show (setup_dialog);

  if (first_time)
    {
      /* Make sure the driver scroller gets positioned correctly... */
      gimp_setup_update ();
      first_time = 0;
    }
}

/*
 *  gimp_new_printer_open_callback() -
 */
static void
gimp_new_printer_open_callback (void)
{
  buttons_pressed = preview_active = 0;
  gtk_entry_set_text(GTK_ENTRY(new_printer_entry), "");
  gtk_widget_show (new_printer_dialog);
}

/*
 *  gimp_setup_ok_callback() -
 */
static void
gimp_setup_ok_callback (void)
{
  buttons_pressed = preview_active = 0;
  stp_set_driver(vars, stp_printer_get_driver(current_printer));
  stp_set_driver(plist[plist_current].v,
		 stp_printer_get_driver(current_printer));

  stp_set_output_to(vars, gtk_entry_get_text (GTK_ENTRY (output_cmd)));
  stp_set_output_to(plist[plist_current].v, stp_get_output_to(vars));

  stp_set_ppd_file(vars, gtk_entry_get_text (GTK_ENTRY (ppd_file)));
  stp_set_ppd_file(plist[plist_current].v, stp_get_ppd_file(vars));

  gimp_plist_callback (NULL, (gpointer) plist_current);

  gtk_widget_hide (setup_dialog);
}

/*
 *  gimp_setup_ok_callback() -
 */
static void
gimp_new_printer_ok_callback (void)
{
  const char *data = gtk_entry_get_text(GTK_ENTRY(new_printer_entry));
  gp_plist_t key;
  buttons_pressed = preview_active = 0;
  initialize_printer(&key);
  (void) strncpy(key.name, data, sizeof(key.name) - 1);
  if (strlen(key.name))
    {
      key.active = 0;
      key.v = stp_allocate_copy(plist[plist_current].v);
      if (add_printer(&key, 1))
	{
	  plist_current = plist_count - 1;
	  gimp_build_printer_combo();

	  stp_set_driver(vars, stp_printer_get_driver(current_printer));
	  stp_set_driver(plist[plist_current].v,
			 stp_printer_get_driver(current_printer));

	  stp_set_output_to(vars, gtk_entry_get_text (GTK_ENTRY (output_cmd)));
	  stp_set_output_to(plist[plist_current].v, stp_get_output_to(vars));

	  stp_set_ppd_file(vars, gtk_entry_get_text (GTK_ENTRY (ppd_file)));
	  stp_set_ppd_file(plist[plist_current].v, stp_get_ppd_file(vars));

	  gimp_plist_callback (NULL, (gpointer) plist_current);
	}
    }

  gtk_widget_hide (new_printer_dialog);
}

/*
 *  gimp_print_driver_callback() - Update the current printer driver...
 */
static void
gimp_print_driver_callback (GtkWidget      *widget, /* I - Driver list */
			    gint            row,
			    gint            column,
			    GdkEventButton *event,
			    gpointer        data)   /* I - Data */
{
  stp_vars_t printvars;
  buttons_pressed = preview_active = 0;
  data = gtk_clist_get_row_data (GTK_CLIST (widget), row);
  current_printer = stp_get_printer_by_index ((gint) data);

  if (strncmp (stp_printer_get_driver(current_printer), "ps", 2) == 0)
    {
      gtk_widget_show (ppd_file);
      gtk_widget_show (ppd_button);
    }
  else
    {
      gtk_widget_hide (ppd_file);
      gtk_widget_hide (ppd_button);
    }
  printvars = stp_printer_get_printvars(current_printer);
  if (stp_get_output_type(printvars) == OUTPUT_COLOR)
    gtk_widget_set_sensitive(output_color, TRUE);
  else
    {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(output_gray), TRUE);
      gtk_widget_set_sensitive(output_color, FALSE);
    }
}

/*
 *  gimp_ppd_browse_callback() -
 */
static void
gimp_ppd_browse_callback (void)
{
  buttons_pressed = preview_active = 0;
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
  buttons_pressed = preview_active = 0;
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
  stp_set_output_to(vars,
	  gtk_file_selection_get_filename (GTK_FILE_SELECTION (file_browser)));

  runme = TRUE;
  gtk_widget_destroy (gimp_color_adjust_dialog);
  gtk_widget_destroy (setup_dialog);
  gtk_widget_destroy (print_dialog);
}

/*
 *  gimp_file_cancel_callback() -
 */
static void
gimp_file_cancel_callback (void)
{
  gtk_widget_hide (file_browser);
  gtk_widget_set_sensitive (gimp_color_adjust_dialog, TRUE);
  gtk_widget_set_sensitive (print_dialog, TRUE);
  gtk_widget_set_sensitive (print_dialog, TRUE);
}

/*
 * gimp_update_adjusted_thumbnail()
 */
void
gimp_update_adjusted_thumbnail (void)
{
  gint      x, y;
  stp_convert_t colorfunc;
  gushort   out[3 * THUMBNAIL_MAXW];
  guchar   *adjusted_data = adjusted_thumbnail_data;
  gfloat    old_density = stp_get_density(vars);

  if (thumbnail_data == 0 || adjusted_thumbnail_data == 0)
    return;

  stp_set_density(vars, 1.0);

  stp_compute_lut (vars, 256);
  colorfunc = stp_choose_colorfunc (stp_get_output_type(vars), thumbnail_bpp,
				    NULL, &adjusted_thumbnail_bpp, vars);

  for (y = 0; y < thumbnail_h; y++)
    {
      (*colorfunc) (vars, thumbnail_data + thumbnail_bpp * thumbnail_w * y,
		     out, NULL, thumbnail_w, thumbnail_bpp, NULL, NULL, NULL,
		     NULL);
      for (x = 0; x < adjusted_thumbnail_bpp * thumbnail_w; x++)
	{
	  *adjusted_data++ = out[x] / 0x0101U;
	}
    }

  stp_free_lut (vars);

  stp_set_density(vars, old_density);

  gimp_redraw_color_swatch ();
  gimp_preview_update ();
}

/*
 *  gimp_preview_update_callback() -
 */
static void
gimp_preview_update (void)
{
  gint	        temp;
  gint          orient;		   /* True orientation of page */
  gdouble	max_ppi_scaling;   /* Maximum PPI for current page size */
  gdouble	min_ppi_scaling;   /* Minimum PPI for current page size */
  gdouble	min_ppi_scaling1;   /* Minimum PPI for current page size */
  gdouble	min_ppi_scaling2;   /* Minimum PPI for current page size */
  static GdkGC	*gc    = NULL;
  static GdkGC  *gcinv = NULL;
  gchar         s[255];
  gint          paper_left, paper_top;
  gdouble	unit_scaler = 72.0;

  (stp_printer_get_printfuncs(current_printer)->media_size)
    (current_printer, vars, &paper_width, &paper_height);

  (stp_printer_get_printfuncs(current_printer)->imageable_area)
    (current_printer, vars, &left, &right, &bottom, &top);

  /* Rationalise things a bit by measuring everything from the top left */
  top = paper_height - top;
  bottom = paper_height - bottom;

  printable_width  = right - left;
  printable_height = bottom - top;

  if (stp_get_orientation(vars) == ORIENT_AUTO)
    {
      if ((printable_width >= printable_height && image_width >= image_height)
	  || (printable_height >= printable_width && image_height >= image_width))
	orient = ORIENT_PORTRAIT;
      else
	orient = ORIENT_LANDSCAPE;
    }
  else
    orient = stp_get_orientation(vars);

  /*
   * Adjust page dimensions depending on the page orientation...
   */

  bottom = paper_height - bottom;
  right = paper_width - right;

  if (orient == ORIENT_LANDSCAPE || orient == ORIENT_SEASCAPE)
    {
      temp              = printable_width;
      printable_width   = printable_height;
      printable_height  = temp;
      temp              = paper_width;
      paper_width       = paper_height;
      paper_height      = temp;
    }
  if (orient == ORIENT_LANDSCAPE)
    {
      temp              = left;
      left              = bottom;
      bottom            = right;
      right             = top;
      top               = temp;
    }
  if (orient == ORIENT_SEASCAPE)
    {
      temp              = left;
      left              = top;
      top               = right;
      right             = bottom;
      bottom            = temp;
    }
  else if (orient == ORIENT_UPSIDEDOWN)
    {
      temp              = left;
      left              = right;
      right		= temp;
      temp              = top;
      top               = bottom;
      bottom		= temp;
    }

  bottom = paper_height - bottom;
  right = paper_width - right;

  if (stp_get_scaling(vars) < 0)
    {
      gdouble twidth;

      min_ppi_scaling1 = 72.0 * (gdouble) image_width / printable_width;
      min_ppi_scaling2 = 72.0 * (gdouble) image_height / printable_height;

      if (min_ppi_scaling1 > min_ppi_scaling2)
	min_ppi_scaling = min_ppi_scaling1;
      else
	min_ppi_scaling = min_ppi_scaling2;

      max_ppi_scaling = min_ppi_scaling * 20;
      if (stp_get_scaling(vars) < 0 &&
	  stp_get_scaling(vars) > -min_ppi_scaling)
	stp_set_scaling(vars, -min_ppi_scaling);
      twidth = (72.0 * (gdouble) image_width / -stp_get_scaling(vars));
      print_width = twidth + .5;
      print_height = (twidth * (gdouble) image_height / image_width) + .5;
      GTK_ADJUSTMENT (scaling_adjustment)->lower = min_ppi_scaling;
      GTK_ADJUSTMENT (scaling_adjustment)->upper = max_ppi_scaling;
      GTK_ADJUSTMENT (scaling_adjustment)->value = -stp_get_scaling(vars);
      if (!suppress_scaling_adjustment)
	{
	  gtk_adjustment_changed (GTK_ADJUSTMENT (scaling_adjustment));
	  suppress_scaling_callback = TRUE;
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scaling_ppi), TRUE);
	  suppress_scaling_callback = FALSE;
	  gtk_adjustment_value_changed (GTK_ADJUSTMENT (scaling_adjustment));
	}
    }
  else
    {
      /* we do stp_get_scaling(vars) % of height or width, whatever is smaller */
      /* this is relative to printable size */
      if (image_width * printable_height > printable_width * image_height)
	/* i.e. if image_width/image_height > printable_width/printable_height */
	/* i.e. if image is wider relative to its height than the width
	   of the printable area relative to its height */
	{
	  gdouble twidth = .5 + printable_width * stp_get_scaling(vars) / 100;

	  print_width = twidth;
	  print_height = twidth * (gdouble) image_height /
	    (gdouble) image_width;
	}
      else
	{
	  gdouble theight = .5 + printable_height * stp_get_scaling(vars) /100;

	  print_height = theight;
	  print_width = theight * (gdouble) image_width /
	    (gdouble) image_height;
	}
    }

  preview_ppi = PREVIEW_SIZE_HORIZ * 72 / paper_width;
  if (PREVIEW_SIZE_VERT * 72 / paper_height < preview_ppi)
    preview_ppi = PREVIEW_SIZE_VERT * 72 / paper_height;
  if (preview_ppi > MAX_PREVIEW_PPI)
    preview_ppi = MAX_PREVIEW_PPI;

  paper_left = (PREVIEW_SIZE_HORIZ - preview_ppi * paper_width / 72) / 2;
  paper_top  = (PREVIEW_SIZE_VERT - preview_ppi * paper_height / 72) / 2;
  printable_left = paper_left +  preview_ppi * left / 72;
  printable_top  = paper_top + preview_ppi * top / 72 ;

  if (preview == NULL || preview->widget.window == NULL)
    return;

  if (stp_get_left(vars) < 0)
    {
      stp_set_left(vars, (paper_width - print_width) / 2 - left);
      if (stp_get_left(vars) < 0)
	stp_set_left(vars, 0);
    }

  /* we leave stp_get_left(vars) etc. relative to printable area */
  if (stp_get_left(vars) > (printable_width - print_width))
    stp_set_left(vars, printable_width - print_width);

  if (stp_get_top(vars) < 0)
    {
      stp_set_top(vars, ((paper_height - print_height) / 2) - top);
      if (stp_get_top(vars) < 0)
	stp_set_top(vars, 0);
    }

  if (stp_get_top(vars) > (printable_height - print_height))
    stp_set_top(vars, printable_height - print_height);

  stp_set_left(plist[plist_current].v, stp_get_left(vars));
  stp_set_top(plist[plist_current].v, stp_get_top(vars));

  if(stp_get_unit(vars)) unit_scaler /= 2.54;
  g_snprintf (s, sizeof (s), "%.2f", (top + stp_get_top(vars)) / unit_scaler);
  gtk_signal_handler_block_by_data (GTK_OBJECT (top_entry), NULL);
  gtk_entry_set_text (GTK_ENTRY (top_entry), s);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (top_entry), NULL);

  g_snprintf (s, sizeof (s), "%.2f",(left + stp_get_left(vars)) / unit_scaler);
  gtk_signal_handler_block_by_data (GTK_OBJECT (left_entry), NULL);
  gtk_entry_set_text (GTK_ENTRY (left_entry), s);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (left_entry), NULL);

  gtk_signal_handler_block_by_data (GTK_OBJECT (bottom_entry), NULL);
  g_snprintf(s, sizeof (s), "%.2f",
	     (top + stp_get_top(vars) + print_height) / unit_scaler);
  gtk_entry_set_text (GTK_ENTRY (bottom_entry), s);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (bottom_entry), NULL);

  gtk_signal_handler_block_by_data (GTK_OBJECT (bottom_border_entry), NULL);
  g_snprintf(s, sizeof (s), "%.2f",
	     (paper_height - (top + stp_get_top(vars) + print_height)) / unit_scaler);
  gtk_entry_set_text (GTK_ENTRY (bottom_border_entry), s);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (bottom_border_entry), NULL);

  gtk_signal_handler_block_by_data (GTK_OBJECT (right_entry), NULL);
  g_snprintf (s, sizeof (s), "%.2f",
	      (left + stp_get_left(vars) + print_width) / unit_scaler);
  gtk_entry_set_text (GTK_ENTRY (right_entry), s);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (right_entry), NULL);

  gtk_signal_handler_block_by_data (GTK_OBJECT (right_border_entry), NULL);
  g_snprintf (s, sizeof (s), "%.2f",
	      (paper_width - (left + stp_get_left(vars) + print_width)) / unit_scaler);
  gtk_entry_set_text (GTK_ENTRY (right_border_entry), s);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (right_border_entry), NULL);

  gtk_signal_handler_block_by_data (GTK_OBJECT (width_entry), NULL);
  g_snprintf (s, sizeof (s), "%.2f", print_width / unit_scaler);
  gtk_entry_set_text (GTK_ENTRY (width_entry), s);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (width_entry), NULL);

  gtk_signal_handler_block_by_data (GTK_OBJECT (height_entry), NULL);
  g_snprintf(s, sizeof (s), "%.2f", print_height / unit_scaler);
  gtk_entry_set_text (GTK_ENTRY (height_entry), s);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (height_entry), NULL);

  gtk_signal_handler_block_by_data (GTK_OBJECT (custom_size_width), NULL);
  g_snprintf (s, sizeof (s), "%.2f", stp_get_page_width(vars) / unit_scaler);
  gtk_entry_set_text (GTK_ENTRY (custom_size_width), s);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (custom_size_width), NULL);

  gtk_signal_handler_block_by_data (GTK_OBJECT (custom_size_height), NULL);
  g_snprintf(s, sizeof (s), "%.2f", stp_get_page_height(vars) / unit_scaler);
  gtk_entry_set_text (GTK_ENTRY (custom_size_height), s);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (custom_size_height), NULL);

  /* draw image */
  if (suppress_preview_update)
    return;
  {
    gint ox, oy, u;
    gint preview_x = 1 + printable_left + preview_ppi * stp_get_left(vars) / 72;
    gint preview_y = 1 + printable_top + preview_ppi * stp_get_top(vars) / 72;
    gint preview_w = MAX (1, (preview_ppi * print_width) / 72);
    gint preview_h = MAX (1, (preview_ppi * print_height) / 72);

    guchar *preview_data;

    gint v_denominator = preview_h > 1 ? preview_h - 1 : 1;
    gint v_numerator = (thumbnail_h - 1) % v_denominator;
    gint v_whole = (thumbnail_h - 1) / v_denominator;
    gint v_cur = 0;
    gint v_last = -1;
    gint v_error = v_denominator / 2;
    gint y = 0;

    preview_data = g_malloc(3 * preview_h * preview_w);
    while (y < preview_h)
      {
	if (v_cur == v_last)
	  {
	    memcpy (preview_data + adjusted_thumbnail_bpp * preview_w * y,
		    preview_data + adjusted_thumbnail_bpp * preview_w * (y - 1),
		    adjusted_thumbnail_bpp * preview_w);
	  }
	else
	  {
	    guchar *inbuf = adjusted_thumbnail_data - adjusted_thumbnail_bpp
	      + adjusted_thumbnail_bpp * thumbnail_w * v_cur;
	    guchar *outbuf = preview_data
	      + adjusted_thumbnail_bpp * preview_w * y;

	    gint h_denominator = preview_w > 1 ? preview_w - 1 : 1;
	    gint h_numerator = (thumbnail_w - 1) % h_denominator;
	    gint h_whole = (thumbnail_w - 1) / h_denominator;
	    gint h_cur = 0;
	    gint h_last = -1;
	    gint h_error = h_denominator / 2;
	    gint x = 0;

	    v_last = v_cur;
	    while (x < preview_w)
	      {
		if (h_cur == h_last)
		  {
		    if (adjusted_thumbnail_bpp == 1)
		      {
			outbuf[0] = outbuf[-1];
			outbuf++;
		      }
		    else
		      {
			outbuf[0] = outbuf[-3];
			outbuf[1] = outbuf[-2];
			outbuf[2] = outbuf[-1];
			outbuf += 3;
		      }
		  }
		else
		  {
		    inbuf += adjusted_thumbnail_bpp * (h_cur - h_last);
		    h_last = h_cur;
		    outbuf[0] = inbuf[0];
		    outbuf++;
		    if (adjusted_thumbnail_bpp == 3)
		      {
			outbuf[0] = inbuf[1];
			outbuf[1] = inbuf[2];
			outbuf += 2;
		      }
		  }
		x++;
		h_cur += h_whole;
		h_error += h_numerator;
		if (h_error >= h_denominator)
		  {
		    h_error -= h_denominator;
		    h_cur++;
		  }
	      }
	  }
	y++;
	v_cur += v_whole;
	v_error += v_numerator;
	if (v_error >= v_denominator)
	  {
	    v_error -= v_denominator;
	    v_cur++;
	  }
      }

    gdk_window_clear (preview->widget.window);

    if (gc == NULL)
      {
	gc = gdk_gc_new (preview->widget.window);
	gcinv = gdk_gc_new (preview->widget.window);
	gdk_gc_set_function (gcinv, GDK_INVERT);
      }

    /* draw paper frame */
    gdk_draw_rectangle (preview->widget.window, gc, 0,
			paper_left, paper_top,
			preview_ppi * paper_width / 72,
			preview_ppi * paper_height / 72);

    /* draw printable frame */
    gdk_draw_rectangle (preview->widget.window, gc, 0,
			printable_left, printable_top,
			preview_ppi * printable_width / 72,
			preview_ppi * printable_height / 72);

    if (adjusted_thumbnail_bpp == 1)
      gdk_draw_gray_image (preview->widget.window, gc,
			   preview_x, preview_y, preview_w, preview_h,
			   GDK_RGB_DITHER_NORMAL, preview_data, preview_w);
    else
      gdk_draw_rgb_image (preview->widget.window, gc,
			  preview_x, preview_y, preview_w, preview_h,
			  GDK_RGB_DITHER_NORMAL, preview_data, 3 * preview_w);

    /* draw orientation arrow pointing to top-of-paper */
    u = preview_ppi/2;
    ox = paper_left + preview_ppi * paper_width / 72 / 2;
    oy = paper_top + preview_ppi * paper_height / 72 / 2;
    if (orient == ORIENT_LANDSCAPE)
      {
	ox += preview_ppi * paper_width / 72 / 4;
	if (ox > paper_left + preview_ppi * paper_width / 72 - u)
	  ox = paper_left + preview_ppi * paper_width / 72 - u;
	gdk_draw_line (preview->widget.window, gcinv, ox + u, oy, ox, oy - u);
	gdk_draw_line (preview->widget.window, gcinv, ox + u, oy, ox, oy + u);
	gdk_draw_line (preview->widget.window, gcinv, ox + u, oy, ox - u, oy);
      }
    else if (orient == ORIENT_SEASCAPE)
      {
	ox -= preview_ppi * paper_width / 72 / 4;
	if (ox < paper_left + u)
	  ox = paper_left + u;
	gdk_draw_line (preview->widget.window, gcinv, ox - u, oy, ox, oy - u);
	gdk_draw_line (preview->widget.window, gcinv, ox - u, oy, ox, oy + u);
	gdk_draw_line (preview->widget.window, gcinv, ox - u, oy, ox + u, oy);
      }
    else if (orient == ORIENT_UPSIDEDOWN)
      {
	oy += preview_ppi * paper_height / 72 / 4;
	if (oy > paper_top + preview_ppi * paper_height / 72 - u)
	  oy = paper_top + preview_ppi * paper_height / 72 - u;
	gdk_draw_line (preview->widget.window, gcinv, ox, oy + u, ox - u, oy);
	gdk_draw_line (preview->widget.window, gcinv, ox, oy + u, ox + u, oy);
	gdk_draw_line (preview->widget.window, gcinv, ox, oy + u, ox, oy - u);
      }
    else /* (orient == ORIENT_PORTRAIT) */
      {
	oy -= preview_ppi * paper_height / 72 / 4;
	if (oy < paper_top + u)
	  oy = paper_top + u;
	gdk_draw_line (preview->widget.window, gcinv, ox, oy - u, ox - u, oy);
	gdk_draw_line (preview->widget.window, gcinv, ox, oy - u, ox + u, oy);
	gdk_draw_line (preview->widget.window, gcinv, ox, oy - u, ox, oy + u);
      }
    g_free(preview_data);
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
  if (event->type == GDK_BUTTON_PRESS)
    {
      if (preview_active == 0)
	{
	  mouse_x = event->x;
	  mouse_y = event->y;
	  old_left = stp_get_left(vars);
	  old_top = stp_get_top(vars);
	  mouse_button = event->button;
	  buttons_mask = 1 << event->button;
	  buttons_pressed++;
	  preview_active = 1;
	}
      else if (preview_active == 1)
	{
	  if ((buttons_mask & (1 << event->button)) == 0)
	    {
	      preview_active = -1;
	      stp_set_left(vars, old_left);
	      stp_set_top(vars, old_top);
	      stp_set_left(plist[plist_current].v, stp_get_left(vars));
	      stp_set_top(plist[plist_current].v, stp_get_top(vars));
	      gimp_preview_update ();
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
  else
    {
      buttons_pressed--;
      buttons_mask &= ~(1 << event->button);
      if (buttons_pressed == 0)
	preview_active = 0;
    }
}

/*
 *  gimp_preview_motion_callback() -
 */
static void
gimp_preview_motion_callback (GtkWidget      *widget,
			      GdkEventMotion *event,
			      gpointer        data)
{
  if (preview_active != 1)
    return;
  if (stp_get_left(vars) < 0 || stp_get_top(vars) < 0)
    {
      stp_set_left(vars, 72 * (printable_width - print_width) / 20);
      stp_set_top(vars, 72 * (printable_height - print_height) / 20);
    }

  if (mouse_button == 2)
    {
      int changes = 0;
      int x_threshold = MAX (1, (preview_ppi * print_width) / 72);
      int y_threshold = MAX (1, (preview_ppi * print_height) / 72);

      while (event->x - mouse_x >= x_threshold)
	{
	  if (stp_get_left(vars) + (print_width * 2) <= right)
	    {
	      stp_set_left(vars, stp_get_left(vars) + print_width);
	      mouse_x += x_threshold;
	      changes = 1;
	    }
	  else
	    break;
	}
      while (mouse_x - event->x >= x_threshold)
	{
	  if (stp_get_left(vars) >= print_width)
	    {
	      stp_set_left(vars, stp_get_left(vars) - print_width);
	      mouse_x -= x_threshold;
	      changes = 1;
	    }
	  else
	    break;
	}

      while (event->y - mouse_y >= y_threshold)
	{
	  if (stp_get_top(vars) + (print_height * 2) <= bottom)
	    {
	      stp_set_top(vars, stp_get_top(vars) + print_height);
	      mouse_y += y_threshold;
	      changes = 1;
	    }
	  else
	    break;
	}
      while (mouse_y - event->y >= y_threshold)
	{
	  if (stp_get_top(vars) >= print_height)
	    {
	      stp_set_top(vars, stp_get_top(vars) - print_height);
	      mouse_y -= y_threshold;
	      changes = 1;
	    }
	  else
	    break;
	}
      if (!changes)
	return;
    }
  else
    {
      int old_top = stp_get_top(vars);
      int old_left = stp_get_left(vars);
      int new_top = old_top;
      int new_left = old_left;
      int changes = 0;
      if (mouse_button == 1)
	{
	  new_top += 72 * (event->y - mouse_y) / preview_ppi;
	  new_left += 72 * (event->x - mouse_x) / preview_ppi;
	}
      else
	{
	  new_top += event->y - mouse_y;
	  new_left += event->x - mouse_x;
	}
      if (new_top < 0)
	new_top = 0;
      if (new_top > (bottom - top) - print_height)
	new_top = (bottom - top) - print_height;
      if (new_left < 0)
	new_left = 0;
      if (new_left > (right - left) - print_width)
	new_left = (right - left) - print_width;

      if (new_top != old_top)
	{
	  stp_set_top(vars, new_top);
	  changes = 1;
	}
      if (new_left != old_left)
	{
	  stp_set_left(vars, new_left);
	  changes = 1;
	}
      mouse_x = event->x;
      mouse_y = event->y;
      if (!changes)
	return;
    }

  stp_set_left(plist[plist_current].v, stp_get_left(vars));
  stp_set_top(plist[plist_current].v, stp_get_top(vars));

  gimp_preview_update ();

}

