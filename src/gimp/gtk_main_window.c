/*
 * "$Id$"
 *
 *   Main window code for Print plug-in for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu), and Steve Miller (smiller@rni.net)
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

#ifndef NEW_UI_ONLY

#define MAX_PREVIEW_PPI        (20)

#include "print-intl.h"
#include <math.h>

/*
 * Constants for GUI...
 */
#define PREVIEW_SIZE_VERT  240 /* Assuming max media size of 24" A2 */
#define PREVIEW_SIZE_HORIZ 240 /* Assuming max media size of 24" A2 */

extern stp_vars_t vars;
extern int plist_count;	     /* Number of system printers */
extern int plist_current;    /* Current system printer */
extern gp_plist_t  *plist;       /* System printers */
extern gint32 image_ID;
extern const char *image_filename;
extern int image_width;
extern int image_height;
extern stp_printer_t current_printer;
extern int runme;
extern int saveme;
extern GtkWidget* gtk_color_adjust_dialog;
extern void gtk_do_color_updates(void);
extern GtkWidget* dither_algo_menu;  /* dither menu */

void  printrc_save(void);

/***
 * Main window widgets
 ***/
static GtkWidget* print_dialog;           /* Print dialog window */
static GtkWidget* recenter_button;
static GtkWidget* left_entry;
static GtkWidget* right_entry;
static GtkWidget* width_entry;
static GtkWidget* top_entry;
static GtkWidget* bottom_entry;
static GtkWidget* height_entry;
static GtkWidget* unit_inch;
static GtkWidget* unit_cm;
static int	 ignore_combo_callback = 0;
static GtkWidget* media_size_combo=NULL;  /* Media size combo box */
static GtkWidget* media_type;             /* Media type option button */
static GtkWidget* media_type_menu=NULL;   /* Media type menu */
static GtkWidget* media_source;           /* Media source option button */
static GtkWidget* media_source_menu=NULL; /* Media source menu */
static GtkWidget* ink_type;               /* Ink type option button */
static GtkWidget* ink_type_menu=NULL;     /* Ink type menu */
static GtkWidget* resolution;             /* Resolution option button */
static GtkWidget* resolution_menu=NULL;   /* Resolution menu */
static GtkWidget* scaling_scale;          /* Scale widget for scaling */
static GtkWidget* scaling_entry;          /* Text entry widget for scaling */
static GtkWidget* scaling_percent;        /* Scale by percent */
static GtkWidget* scaling_ppi;            /* Scale by pixels-per-inch */
static GtkWidget* output_gray;            /* Output type toggle, black */
static GtkWidget* output_color;           /* Output type toggle, color */
static GtkWidget* image_line_art;
static GtkWidget* image_solid_tone;
static GtkWidget* image_continuous_tone;
static GtkWidget* image_monochrome;
static GtkWidget* setup_dialog;           /* Setup dialog window */
static GtkWidget* printer_driver;         /* Printer driver widget */
static GtkWidget* printer_crawler;        /* Scrolled Window for menu */
static GtkWidget* ppd_file;               /* PPD file entry */
static GtkWidget* ppd_button;             /* PPD file browse button */
static GtkWidget* output_cmd;             /* Output command text entry */
static GtkWidget* ppd_browser;            /* File selection dialog for PPD files */
static GtkWidget* file_browser;           /* FSD for print files */
static GtkWidget* printandsave_button;
static GtkWidget* adjust_color_button;

static GtkObject* scaling_adjustment;	   /* Adjustment object for scaling */
static int suppress_scaling_adjustment = 0;
static int suppress_scaling_callback = 0;

static int		num_media_types=0;	/* Number of media types */
static char		**media_types;		/* Media type strings */
static int		num_media_sources=0;	/* Number of media sources */
static char		**media_sources;	/* Media source strings */
static int		num_ink_types=0;	/* Number of ink types */
static char		**ink_types;		/* Ink type strings */
static int		num_resolutions=0;	/* Number of resolutions */
static char		**resolutions;		/* Resolution strings */


static GtkDrawingArea	*preview;		/* Preview drawing area widget */
static int		mouse_x,		/* Last mouse X */
	        	mouse_y,		/* Last mouse Y */
			mouse_button;		/* Button being dragged with */

static int 		printable_left;		/* Left pixel column of printable */
static int		printable_top;		/* Top pixel row of printable */
static int		printable_width;		/* Width of printable on screen */
static int		printable_height;		/* Height of printable on screen */
static int		print_width;		/* Printed width of image */
static int		print_height;		/* Printed height of image */
static int		left, right;	        /* Imageable area */
static int              top, bottom;
static int		paper_width, paper_height;	/* Physical width */

static void gtk_scaling_update(GtkAdjustment *);
static void gtk_scaling_callback(GtkWidget *);
static void gtk_plist_callback(GtkWidget *, gint);
static void gtk_media_size_callback(GtkWidget *, gint);
static void gtk_media_type_callback(GtkWidget *, gint);
static void gtk_media_source_callback(GtkWidget *, gint);
static void gtk_ink_type_callback(GtkWidget *, gint);
static void gtk_resolution_callback(GtkWidget *, gint);
static void gtk_output_type_callback(GtkWidget *, gint);
static void gtk_unit_callback(GtkWidget *, gint);
static void gtk_orientation_callback(GtkWidget *, gint);
static void gtk_printandsave_callback(void);
static void gtk_print_callback(void);
static void gtk_save_callback(void);
static void gtk_cancel_callback(void);
static void gtk_close_callback(void);

static void gtk_setup_open_callback(void);
static void gtk_setup_ok_callback(void);
static void gtk_setup_cancel_callback(void);
static void gtk_ppd_browse_callback(void);
static void gtk_ppd_ok_callback(void);
static void gtk_ppd_cancel_callback(void);
static void gtk_print_driver_callback(GtkWidget *, gint, gint, GdkEventButton *, gpointer);

static void gtk_file_ok_callback(void);
static void gtk_file_cancel_callback(void);

static void gtk_preview_update(void);
static void gtk_preview_button_callback(GtkWidget *, GdkEventButton *);
static void gtk_preview_motion_callback(GtkWidget *, GdkEventMotion *);
static void gtk_position_callback(GtkWidget *);
static void gtk_image_type_callback(GtkWidget *widget,
				    gint      data);
static void gtk_show_adjust_button_callback(GtkWidget *);

extern void gtk_create_color_adjust_window(void);

static GtkWidget* table;      /* Table "container" for controls */
static GtkWidget* dialog;     /* Dialog window */
extern void gtk_build_dither_menu(void);

static int preview_ppi = 10;

static char *
c_strdup(const char *s)
{
  char *ret = xmalloc(strlen(s) + 1);
  strcpy(ret, s);
  return ret;
}

/*****************************************************************************
 *
 * gtk_create_main_window()
 *
 * NOTES:
 *
 *****************************************************************************/
void gtk_create_main_window(void)
{
    int        i;        /* Looping var */
    char       s[100];     /* Text string */
    GtkWidget* label;      /* Label string */
    GtkWidget* hbbox;      /* button_box for OK/Cancel buttons */
    GtkWidget* button;     /* OK/Cancel buttons */
    GtkWidget* scale;      /* Scale widget */
    GtkWidget* entry;      /* Text entry widget */
    GtkWidget* menu;       /* Menu of sizes */
    GtkWidget* list;       /* List of drivers */
    GtkWidget* item;       /* Menu item */
    GtkWidget* option;     /* Option menu button */
    GtkWidget* combo;      /* Combo box */
    GtkWidget* box;        /* Box container */
    GtkObject* scale_data; /* Scale data (limits) */
    GSList*    group;      /* Grouping for output type */
    GSList*    image_type_group;  /* Grouping for image type */

    stp_printer_t the_printer = stp_get_printer_by_index(0);

    static char   *orients[] =    /* Orientation strings */
    {
	N_("Auto"),
	N_("Portrait"),
	N_("Landscape"),
	N_("Upside down"),
	N_("Seascape")
    };
    gchar *plug_in_name;

    /***
     * Create the main dialog
     ***/
    print_dialog = dialog = gtk_dialog_new();

    plug_in_name = g_strdup_printf (_("%s - Print v%s"),
                                    image_filename, PLUG_IN_VERSION);
    gtk_window_set_title(GTK_WINDOW(dialog), plug_in_name);
    g_free (plug_in_name);

    gtk_window_set_wmclass(GTK_WINDOW(dialog), "print", "Gimp");
    gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
    gtk_container_border_width(GTK_CONTAINER(dialog), 0);
    gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
		       (GtkSignalFunc)gtk_close_callback, NULL);

    /*
     * Top-level table for dialog...
     */
    table = gtk_table_new(14, 4, FALSE);
    gtk_container_border_width(GTK_CONTAINER(table), 6);
    gtk_table_set_col_spacings(GTK_TABLE(table), 4);
    gtk_table_set_row_spacings(GTK_TABLE(table), 8);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
		       table,
		       FALSE,
		       FALSE,
		       0);
    gtk_widget_show(table);

    /*
     * Drawing area for printable preview...
     */
    preview = (GtkDrawingArea *)gtk_drawing_area_new();
    gtk_drawing_area_size(preview, PREVIEW_SIZE_VERT, PREVIEW_SIZE_HORIZ);
    gtk_table_attach(GTK_TABLE(table),
		     (GtkWidget *)preview,
		     0, 2,
		     0, 7,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_signal_connect(GTK_OBJECT((GtkWidget *)preview),
		       "expose_event",
		       (GtkSignalFunc)gtk_preview_update,
		       NULL);
    gtk_signal_connect(GTK_OBJECT((GtkWidget *)preview),
		       "button_press_event",
		       (GtkSignalFunc)gtk_preview_button_callback,
		       NULL);
    gtk_signal_connect(GTK_OBJECT((GtkWidget *)preview),
		       "button_release_event",
		       (GtkSignalFunc)gtk_preview_button_callback,
		       NULL);
    gtk_signal_connect(GTK_OBJECT((GtkWidget *)preview),
		       "motion_notify_event",
		       (GtkSignalFunc)gtk_preview_motion_callback,
		       NULL);
    gtk_widget_show((GtkWidget *)preview);

    gtk_widget_set_events((GtkWidget *)preview,
			  GDK_EXPOSURE_MASK | GDK_BUTTON_MOTION_MASK |
			  GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

    recenter_button = button = gtk_button_new_with_label(_("Center Image"));
    gtk_table_attach(GTK_TABLE(table),
		     button,
		     1, 2,
		     7, 8,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc)gtk_position_callback, NULL);
    gtk_widget_show(button);

    label = gtk_label_new(_("Left:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label, 0, 1, 8, 9, GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show(label);
    box = gtk_hbox_new(FALSE, 8);
    gtk_table_attach(GTK_TABLE(table),
		     box, 1, 2, 8, 9, GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show(box);
    left_entry = entry = gtk_entry_new();
    sprintf(s, "%.3f", fabs(stp_get_left(vars)));
    gtk_entry_set_text(GTK_ENTRY(entry), s);
    gtk_signal_connect(GTK_OBJECT(entry), "activate",
		       (GtkSignalFunc)gtk_position_callback, NULL);
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
    gtk_widget_set_usize(entry, 60, 0);
    gtk_widget_show(entry);

    label = gtk_label_new(_("Top:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label, 2, 3, 8, 9, GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show(label);
    box = gtk_hbox_new(FALSE, 8);
    gtk_table_attach(GTK_TABLE(table),
		     box, 3, 4, 8, 9, GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show(box);
    top_entry = entry = gtk_entry_new();
    sprintf(s, "%.3f", fabs(stp_get_top(vars)));
    gtk_entry_set_text(GTK_ENTRY(entry), s);
    gtk_signal_connect(GTK_OBJECT(entry), "activate",
		       (GtkSignalFunc)gtk_position_callback, NULL);
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
    gtk_widget_set_usize(entry, 60, 0);
    gtk_widget_show(entry);

    label = gtk_label_new(_("Right:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label, 0, 1, 9, 10, GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show(label);
    box = gtk_hbox_new(FALSE, 8);
    gtk_table_attach(GTK_TABLE(table),
		     box, 1, 2, 9, 10, GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show(box);
    right_entry = entry = gtk_entry_new();
    sprintf(s, "%.3f", fabs(stp_get_left(vars)));
    gtk_entry_set_text(GTK_ENTRY(entry), s);
    gtk_signal_connect(GTK_OBJECT(entry), "activate",
		       (GtkSignalFunc)gtk_position_callback, NULL);
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
    gtk_widget_set_usize(entry, 60, 0);
    gtk_widget_show(entry);

    label = gtk_label_new(_("Bottom:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label, 2, 3, 9, 10, GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show(label);
    box = gtk_hbox_new(FALSE, 8);
    gtk_table_attach(GTK_TABLE(table),
		     box, 3, 4, 9, 10, GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show(box);
    bottom_entry = entry = gtk_entry_new();
    sprintf(s, "%.3f", fabs(stp_get_left(vars)));
    gtk_entry_set_text(GTK_ENTRY(entry), s);
    gtk_signal_connect(GTK_OBJECT(entry), "activate",
		       (GtkSignalFunc)gtk_position_callback, NULL);
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
    gtk_widget_set_usize(entry, 60, 0);
    gtk_widget_show(entry);

    label = gtk_label_new(_("Width:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label, 0, 1, 10, 11, GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show(label);
    box = gtk_hbox_new(FALSE, 8);
    gtk_table_attach(GTK_TABLE(table),
		     box, 1, 2, 10, 11, GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show(box);
    width_entry = entry = gtk_entry_new();
    sprintf(s, "%.3f", fabs(stp_get_left(vars)));
    gtk_entry_set_text(GTK_ENTRY(entry), s);
    gtk_signal_connect(GTK_OBJECT(entry), "activate",
		       (GtkSignalFunc)gtk_position_callback, NULL);
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
    gtk_widget_set_usize(entry, 60, 0);
    gtk_widget_show(entry);

    label = gtk_label_new(_("Height:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label, 2, 3, 10, 11, GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show(label);
    box = gtk_hbox_new(FALSE, 8);
    gtk_table_attach(GTK_TABLE(table),
		     box, 3, 4, 10, 11, GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show(box);
    height_entry = entry = gtk_entry_new();
    sprintf(s, "%.3f", fabs(stp_get_left(vars)));
    gtk_entry_set_text(GTK_ENTRY(entry), s);
    gtk_signal_connect(GTK_OBJECT(entry), "activate",
		       (GtkSignalFunc)gtk_position_callback, NULL);
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
    gtk_widget_set_usize(entry, 60, 0);
    gtk_widget_show(entry);

    label = gtk_label_new(_("Units:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label, 0, 1, 13, 14, GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show(label);
    box = gtk_hbox_new(FALSE, 8);
    gtk_table_attach(GTK_TABLE(table),
		     box,
		     1, 2,
		     13, 14,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(box);
    unit_inch = button = gtk_radio_button_new_with_label(NULL, _("Inch"));
    group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
    if (stp_get_unit(vars) == 0)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    unit_cm = button = gtk_radio_button_new_with_label(group, _("cm"));
    if (stp_get_unit(vars))
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    gtk_signal_connect(GTK_OBJECT(unit_inch), "toggled",
		       (GtkSignalFunc)gtk_unit_callback, (gpointer)0);
    gtk_signal_connect(GTK_OBJECT(unit_cm), "toggled",
		       (GtkSignalFunc)gtk_unit_callback, (gpointer)1);

    /***
     * Media size option menu...
     ***/
    label = gtk_label_new(_("Media Size:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label,
		     2, 3,
		     1, 2,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(label);

    box = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table),
		     box,
		     3, 4,
		     1, 2,
		     GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show(box);

    media_size_combo = combo = gtk_combo_new();
    gtk_box_pack_start(GTK_BOX(box), combo, FALSE, FALSE, 0);

    /*
     * Media type option menu...
     */
    label = gtk_label_new(_("Media Type:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label,
		     2, 3,
		     2, 3,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(label);

    box = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table),
		     box,
		     3, 4,
		     2, 3,
		     GTK_FILL,
		     GTK_FILL, 0, 0);
    gtk_widget_show(box);

    media_type = option = gtk_option_menu_new();
    gtk_box_pack_start(GTK_BOX(box), option, FALSE, FALSE, 0);

    /*
     * Media source option menu...
     */
    label = gtk_label_new(_("Media Source:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label,
		     2, 3,
		     3, 4,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(label);

    box = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table),
		     box,
		     3, 4,
		     3, 4,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(box);

    media_source = option = gtk_option_menu_new();
    gtk_box_pack_start(GTK_BOX(box), option, FALSE, FALSE, 0);

    /*
     * Ink type option menu...
     */
    label = gtk_label_new(_("Ink Type:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label,
		     2, 3,
		     4, 5,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(label);

    box = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table),
		     box,
		     3, 4,
		     4, 5,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(box);

    ink_type = option = gtk_option_menu_new();
    gtk_box_pack_start(GTK_BOX(box), option, FALSE, FALSE, 0);

    /*
     * Orientation option menu...
     */
    label = gtk_label_new(_("Orientation:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label,
		     2, 3,
		     5, 6,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(label);

    menu = gtk_menu_new();
    for (i = 0; i < (int)(sizeof(orients) / sizeof(orients[0])); i ++)
    {
	item = gtk_menu_item_new_with_label(gettext(orients[i]));
	gtk_menu_append(GTK_MENU(menu), item);
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   (GtkSignalFunc)gtk_orientation_callback,
			   (gpointer)(i - 1));
	gtk_widget_show(item);
    }

    box = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table),
		     box,
		     3, 4,
		     5, 6,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(box);

    option = gtk_option_menu_new();
    gtk_box_pack_start(GTK_BOX(box), option, FALSE, FALSE, 0);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(option), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(option), stp_get_orientation(vars) + 1);
    gtk_widget_show(option);

    /*
     * Resolution option menu...
     */
    label = gtk_label_new(_("Resolution:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label,
		     2, 3,
		     6, 7,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(label);

    box = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table),
		     box,
		     3, 4,
		     6, 7,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(box);

    resolution = option = gtk_option_menu_new();
    gtk_box_pack_start(GTK_BOX(box), option, FALSE, FALSE, 0);

    /*
     * Output type toggles...
     */
    label = gtk_label_new(_("Output Type:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label,
		     2, 3,
		     7, 8,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(label);

    box = gtk_hbox_new(FALSE, 8);
    gtk_table_attach(GTK_TABLE(table),
		     box,
		     3, 4,
		     7, 8,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(box);

    output_gray = button = gtk_radio_button_new_with_label(NULL, _("B&W"));
    group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
    if (stp_get_output_type(vars) == 0)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_signal_connect(GTK_OBJECT(button), "toggled",
		       (GtkSignalFunc)gtk_output_type_callback,
		       (gpointer)OUTPUT_GRAY);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    output_color = button = gtk_radio_button_new_with_label(group, _("Color"));
    if (stp_get_output_type(vars) == 1)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_signal_connect(GTK_OBJECT(button), "toggled",
		       (GtkSignalFunc)gtk_output_type_callback,
		       (gpointer)OUTPUT_COLOR);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    /*
     * Image type
     */
    label = gtk_label_new(_("Image Type:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label,
		     0, 1,
		     11, 12,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(label);

    box = gtk_hbox_new(FALSE, 8);
    gtk_table_attach(GTK_TABLE(table),
		     box,
		     1, 4,
		     11, 12,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(box);

    image_line_art= button =
	gtk_radio_button_new_with_label(NULL, _("Line Art"));
    image_type_group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
    if (stp_get_image_type(vars) == IMAGE_LINE_ART)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_signal_connect(GTK_OBJECT(button), "toggled",
		       (GtkSignalFunc)gtk_image_type_callback,
		       (gpointer)IMAGE_LINE_ART);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    image_solid_tone= button =
	gtk_radio_button_new_with_label(image_type_group, _("Solid Colors"));
    if (stp_get_image_type(vars) == IMAGE_SOLID_TONE)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_signal_connect(GTK_OBJECT(button), "toggled",
		       (GtkSignalFunc)gtk_image_type_callback,
		       (gpointer)IMAGE_SOLID_TONE);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    image_continuous_tone= button =
	gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(button)),
					_("Photograph"));
    if (stp_get_image_type(vars) == IMAGE_CONTINUOUS)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_signal_connect(GTK_OBJECT(button), "toggled",
		       (GtkSignalFunc)gtk_image_type_callback,
		       (gpointer)IMAGE_CONTINUOUS);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    image_monochrome= button =
	gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(button)),
					_("Monochrome"));
    if (stp_get_image_type(vars) == IMAGE_MONOCHROME)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_signal_connect(GTK_OBJECT(button), "toggled",
		       (GtkSignalFunc)gtk_image_type_callback,
		       (gpointer)IMAGE_MONOCHROME);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    /*
     * Scaling...
     */
    label = gtk_label_new(_("Scaling:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label,
		     0, 1,
		     12, 13,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(label);

    box = gtk_hbox_new(FALSE, 8);
    gtk_table_attach(GTK_TABLE(table),
		     box,
		     1, 4,
		     12, 13,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(box);

    (*stp_printer_get_printfuncs(current_printer)->media_size) (current_printer, vars,
						&paper_width, &paper_height);

    (*stp_printer_get_printfuncs(current_printer)->imageable_area) (current_printer, vars,
						    &left, &right,
						    &bottom, &top);

    /* Rationalise things a bit by measuring everything from the top left */
    top = paper_height - top;
    bottom = paper_height - bottom;

    printable_width  = right - left;
    printable_height = bottom - top;

    if (stp_get_scaling(vars) < 0.0)
      {
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
	max_ppi_scaling = min_ppi_scaling * 20;
	scaling_adjustment = scale_data =
	  gtk_adjustment_new(-stp_get_scaling(vars), min_ppi_scaling,
			     max_ppi_scaling + 1, 1.0, 1.0, 1.0);
      }
    else
	scaling_adjustment = scale_data =
	    gtk_adjustment_new(stp_get_scaling(vars), 5.0, 101.0, 1.0, 1.0, 1.0);

    gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
		       (GtkSignalFunc)gtk_scaling_update, NULL);

    scaling_scale = scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
    gtk_box_pack_start(GTK_BOX(box), scale, FALSE, FALSE, 0);
    gtk_widget_set_usize(scale, 200, 0);
    gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
    gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
    gtk_widget_show(scale);

    scaling_entry = entry = gtk_entry_new();
    sprintf(s, "%.1f", fabs(stp_get_scaling(vars)));
    gtk_entry_set_text(GTK_ENTRY(entry), s);
    gtk_signal_connect(GTK_OBJECT(entry), "activate",
		       (GtkSignalFunc)gtk_scaling_callback, NULL);
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
    gtk_widget_set_usize(entry, 60, 0);
    gtk_widget_show(entry);

    scaling_percent = button = gtk_radio_button_new_with_label(NULL,
							       _("Percent"));
    group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
    if (stp_get_scaling(vars) > 0.0)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    scaling_ppi = button = gtk_radio_button_new_with_label(group, _("PPI"));
    if (stp_get_scaling(vars) < 0.0)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    gtk_signal_connect(GTK_OBJECT(scaling_percent), "toggled",
		       (GtkSignalFunc)gtk_scaling_callback, NULL);
    gtk_signal_connect(GTK_OBJECT(scaling_ppi), "toggled",
		       (GtkSignalFunc)gtk_scaling_callback, NULL);

    /***
     *  Color adjust button
     ***/
    box = gtk_hbox_new(FALSE, 8);
    gtk_table_attach(GTK_TABLE(table),
		     box,
		     3, 4, 13, 14,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(box);
    adjust_color_button = gtk_button_new_with_label(_("Adjust Color"));
    gtk_signal_connect (GTK_OBJECT (adjust_color_button),
			"clicked",
			(GtkSignalFunc)gtk_show_adjust_button_callback,
			NULL);
    gtk_box_pack_start (GTK_BOX (box), adjust_color_button, FALSE, FALSE, 0);
    gtk_widget_show (adjust_color_button);

    gtk_create_color_adjust_window();

    /*
     * Printer option menu...
     */
    label = gtk_label_new(_("Printer:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label,
		     2, 3,
		     0, 1,
		     GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show(label);

    menu = gtk_menu_new();
    for (i = 0; i < plist_count; i ++)
    {
	if (plist[i].active)
	    item = gtk_menu_item_new_with_label(gettext(plist[i].name));
	else
	{
	    char buf[18];
	    buf[0] = '*';
	    memcpy(buf + 1, plist[i].name, 17);
	    item = gtk_menu_item_new_with_label(gettext(buf));
	}
	gtk_menu_append(GTK_MENU(menu), item);
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   (GtkSignalFunc)gtk_plist_callback,
			   (gpointer)i);
	gtk_widget_show(item);
    }

    box = gtk_hbox_new(FALSE, 8);
    gtk_table_attach(GTK_TABLE(table),
		     box,
		     3, 4,
		     0, 1,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(box);

    option = gtk_option_menu_new();
    gtk_box_pack_start(GTK_BOX(box), option, FALSE, FALSE, 0);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(option), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(option), plist_current);
    gtk_widget_show(option);

    button = gtk_button_new_with_label(_("Setup"));
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc)gtk_setup_open_callback, NULL);
    gtk_widget_show(button);

    /*
     * Print, cancel buttons...
     */
    gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->
						   action_area), 2);
    gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dialog)->action_area),
			     FALSE);
    hbbox = gtk_hbutton_box_new ();
    gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
    gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dialog)->action_area),
		      hbbox,
		      FALSE,
		      FALSE,
		      0);
    gtk_widget_show(hbbox);
    gtk_box_set_homogeneous(GTK_BOX(GTK_DIALOG(dialog)->action_area), FALSE);
    gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(dialog)->action_area), 0);

    button = printandsave_button =
	gtk_button_new_with_label (_("Print And Save Settings"));
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_signal_connect (GTK_OBJECT (button),
			"clicked",
			(GtkSignalFunc) gtk_printandsave_callback,
			NULL);
    gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
    gtk_widget_grab_default (button);
    gtk_widget_show (button);

    button = gtk_button_new_with_label (_("Print"));
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_signal_connect (GTK_OBJECT (button),
			"clicked",
			(GtkSignalFunc) gtk_print_callback,
			NULL);
    gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    button = gtk_button_new_with_label (_("Save Current Settings"));
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_signal_connect (GTK_OBJECT (button),
			"clicked",
			(GtkSignalFunc) gtk_save_callback,
			NULL);
    gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    button = gtk_button_new_with_label (_("Cancel"));
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_signal_connect (GTK_OBJECT(button),
			"clicked",
			(GtkSignalFunc) gtk_cancel_callback, NULL);
    gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    /*
     * Setup dialog window...
     */
    setup_dialog = dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), _("Setup"));
    gtk_window_set_wmclass(GTK_WINDOW(dialog), "print", "Gimp");
    gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
    gtk_container_border_width(GTK_CONTAINER(dialog), 0);
    gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
		       (GtkSignalFunc)gtk_setup_cancel_callback, NULL);

    /*
     * Top-level table for dialog...
     */
    table = gtk_table_new(5, 2, FALSE);
    gtk_container_border_width(GTK_CONTAINER(table), 6);
    gtk_table_set_col_spacings(GTK_TABLE(table), 4);
    gtk_table_set_row_spacings(GTK_TABLE(table), 8);
    gtk_table_set_row_spacing (GTK_TABLE (table), 0, 100);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
		       table,
		       FALSE,
		       FALSE,
		       0);
    gtk_widget_show(table);

    /*
     * Printer driver option menu...
     */
    label = gtk_label_new(_("Driver:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label,
		     0, 1,
		     0, 2,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(label);

    printer_crawler = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (printer_crawler),
				   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    printer_driver = list = gtk_clist_new(1);
    gtk_clist_set_selection_mode(GTK_CLIST(list), GTK_SELECTION_SINGLE);
    gtk_signal_connect(GTK_OBJECT(list), "select_row",
		       (GtkSignalFunc)gtk_print_driver_callback, NULL);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW (printer_crawler), list);
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
    gtk_table_attach(GTK_TABLE(table),
		     printer_crawler,
		     1, 3,
		     0, 2,
		     GTK_FILL, GTK_FILL,
		     0, 0);
	gtk_widget_show (printer_crawler);

    /*
     * PPD file...
     */
    label = gtk_label_new(_("PPD File:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label,
		     0, 1,
		     2, 3,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(label);

    box = gtk_hbox_new(FALSE, 8);
    gtk_table_attach(GTK_TABLE(table),
		     box,
		     1, 2,
		     2, 3,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(box);

    ppd_file = entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(box), entry, TRUE, TRUE, 0);
    gtk_widget_show(entry);

    ppd_button = button = gtk_button_new_with_label(_("Browse"));
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc)gtk_ppd_browse_callback, NULL);
    gtk_widget_show(button);

    /*
     * Print command...
     */
    label = gtk_label_new(_("Command:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label,
		     0, 1,
		     3, 4,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(label);

    output_cmd = entry = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table),
		     entry,
		     1, 2,
		     3, 4,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(entry);

    /*
     * OK, cancel buttons...
     */
    gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG (dialog)->
						 action_area),
				   2);
    gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dialog)->action_area),
			     FALSE);
    hbbox = gtk_hbutton_box_new ();
    gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
    gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dialog)->action_area),
		      hbbox,
		      FALSE,
		      FALSE,
		      0);
    gtk_widget_show (hbbox);

    button = gtk_button_new_with_label (_("OK"));
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			(GtkSignalFunc) gtk_setup_ok_callback, NULL);
    gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
    gtk_widget_grab_default (button);
    gtk_widget_show (button);

    button = gtk_button_new_with_label (_("Cancel"));
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc)gtk_setup_cancel_callback, NULL);
    gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    /*
     * Output file selection dialog...
     */
    file_browser = gtk_file_selection_new(_("Print To File?"));
    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(file_browser)->ok_button),
		       "clicked", (GtkSignalFunc)gtk_file_ok_callback, NULL);
    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(file_browser)->
				  cancel_button),
		       "clicked",
		       (GtkSignalFunc)gtk_file_cancel_callback,
		       NULL);

    /*
     * PPD file selection dialog...
     */
    ppd_browser = gtk_file_selection_new(_("PPD File?"));
    gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(ppd_browser));
    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(ppd_browser)->ok_button),
		       "clicked",
		       (GtkSignalFunc)gtk_ppd_ok_callback,
		       NULL);
    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(ppd_browser)->
				  cancel_button),
		       "clicked",
		       (GtkSignalFunc)gtk_ppd_cancel_callback,
		       NULL);

    /*
     * Show the main dialog and wait for the user to do something...
     */
    gtk_plist_callback(NULL, plist_current);
    gtk_widget_show(print_dialog);
}

/****************************************************************************
 *
 * GTK_scaling_update() - Update the scaling scale using the slider.
 *
 ****************************************************************************/
static void gtk_scaling_update(GtkAdjustment *adjustment) /* I - New value */
{
  char	s[255];					/* Text buffer */


  if (stp_get_scaling(vars) != adjustment->value)
  {
    if (GTK_TOGGLE_BUTTON(scaling_ppi)->active)
      stp_set_scaling(vars, -adjustment->value);
    else
      stp_set_scaling(vars, adjustment->value);
    stp_set_scaling(plist[plist_current].v, stp_get_scaling(vars));

    sprintf(s, "%.1f", adjustment->value);

    gtk_signal_handler_block_by_data(GTK_OBJECT(scaling_entry), NULL);
    gtk_entry_set_text(GTK_ENTRY(scaling_entry), s);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(scaling_entry), NULL);

    suppress_scaling_adjustment = 1;
    gtk_preview_update();
    suppress_scaling_adjustment = 0;
  }
}


/****************************************************************************
 *
 * gtk_scaling_callback() - Update the scaling field using the text entry
 *
 ****************************************************************************/
static void gtk_scaling_callback(GtkWidget* widget) /* I - New value */
{
  gdouble	new_value;		/* New scaling value */
  gdouble max_ppi_scaling;
  gdouble min_ppi_scaling, min_ppi_scaling1, min_ppi_scaling2;
  gdouble current_scale;
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
  max_ppi_scaling = min_ppi_scaling * 20;


  if (widget == scaling_entry)
  {
    new_value = atof(gtk_entry_get_text(GTK_ENTRY(widget)));

    if (stp_get_scaling(vars) != new_value)
    {
      if ((new_value >= GTK_ADJUSTMENT(scaling_adjustment)->lower) &&
	  (new_value < GTK_ADJUSTMENT(scaling_adjustment)->upper))
      {
	GTK_ADJUSTMENT(scaling_adjustment)->value = new_value;

	gtk_signal_emit_by_name(scaling_adjustment, "value_changed");
      }
    }
  }
  else if (widget == scaling_ppi)
  {
    if (!(GTK_TOGGLE_BUTTON(scaling_ppi)->active))
      return;
    GTK_ADJUSTMENT (scaling_adjustment)->lower = min_ppi_scaling;
    GTK_ADJUSTMENT (scaling_adjustment)->upper = max_ppi_scaling + 1;

    /*
     * Compute the correct PPI to create an image of the same size
     * as the one measured in percent
     */
    current_scale = GTK_ADJUSTMENT (scaling_adjustment)->value;
    GTK_ADJUSTMENT (scaling_adjustment)->value =
      min_ppi_scaling / (current_scale / 100);
    stp_set_scaling(vars, 0.0);
    stp_set_scaling(plist[plist_current].v, stp_get_scaling(vars));
    gtk_signal_emit_by_name(scaling_adjustment, "value_changed");
  }
  else if (widget == scaling_percent)
  {
    gdouble new_percent;
    if (!(GTK_TOGGLE_BUTTON(scaling_percent)->active))
      return;
    current_scale = GTK_ADJUSTMENT (scaling_adjustment)->value;
    GTK_ADJUSTMENT(scaling_adjustment)->lower = 5.0;
    GTK_ADJUSTMENT(scaling_adjustment)->upper = 101.0;

    new_percent = 100 * min_ppi_scaling / current_scale;
    if (new_percent > 100)
      new_percent = 100;
    if (new_percent < 5)
      new_percent = 5;
    GTK_ADJUSTMENT (scaling_adjustment)->value = new_percent;
    stp_set_scaling(vars, 0.0);
    stp_set_scaling(plist[plist_current].v, stp_get_scaling(vars));
    gtk_signal_emit_by_name(scaling_adjustment, "value_changed");
  }
}

/****************************************************************************
 *
 * gtk_plist_build_menu
 *
 ****************************************************************************/
static void gtk_plist_build_menu(GtkWidget*  option,  /* I - Option button */
				 GtkWidget** menu,    /* IO - Current menu */
				 int       num_items, /* I - Number of items */
				 char**    items,     /* I - Menu items */
				 char*     cur_item,  /* I - Current item */
		        void (*callback)(GtkWidget *, gint)) /* I - Callback */
{
  int		i;	/* Looping var */
  GtkWidget	*item,	/* Menu item */
    *item0 = 0;	/* First menu item */


  if (*menu != NULL)
    {
      gtk_widget_destroy(*menu);
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
      gtk_widget_show(option);
      return;
    }
  else
    {
      gtk_widget_set_sensitive (option, TRUE);
    }

  for (i = 0; i < num_items; i ++)
    {
      item = gtk_menu_item_new_with_label(gettext(items[i]));
      if (i == 0)
	item0 = item;
      gtk_menu_append(GTK_MENU(*menu), item);
      gtk_signal_connect(GTK_OBJECT(item), "activate",
			 (GtkSignalFunc)callback, (gpointer)i);
      gtk_widget_show(item);
    }

  gtk_option_menu_set_menu(GTK_OPTION_MENU(option), *menu);

#ifdef DEBUG
  printf("cur_item = \'%s\'\n", cur_item);
#endif /* DEBUG */

  for (i = 0; i < num_items; i ++)
    {
#ifdef DEBUG
      printf("item[%d] = \'%s\'\n", i, items[i]);
#endif /* DEBUG */

      if (strcmp(items[i], cur_item) == 0)
	{
	  gtk_option_menu_set_history(GTK_OPTION_MENU(option), i);
	  break;
	}
    }

  if (i == num_items)
    {
      gtk_option_menu_set_history(GTK_OPTION_MENU(option), 0);
      gtk_signal_emit_by_name(GTK_OBJECT(item0), "activate");
    }
  gtk_widget_show(option);
}

/****************************************************************************
 *
 * gtk_plist_build_combo
 *
 ****************************************************************************/
static void gtk_plist_build_combo(GtkWidget*  combo,   /* I - Combo widget */
				  int       num_items, /* I - Number of items */
				  char**    items,     /* I - Menu items */
				  char*     cur_item,  /* I - Current item */
		        void (*callback)(GtkWidget *, gint)) /* I - Callback */
{
  int		i;	/* Looping var */
  GList		*list = 0;
  GtkEntry	*entry = GTK_ENTRY(GTK_COMBO(combo)->entry);


  if (num_items == 0)
    {
      list = g_list_append(list, _("Standard"));
      gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);
      g_list_free(list);
      gtk_widget_set_sensitive(combo, FALSE);
      gtk_entry_set_editable(entry, FALSE);
      gtk_widget_show(combo);
      return;
    }

  for (i = 0; i < num_items; i ++)
      list = g_list_append(list, gettext(items[i]));

  ignore_combo_callback = 1;
  gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);
  ignore_combo_callback = 0;

  gtk_signal_connect(GTK_OBJECT(entry), "changed", (GtkSignalFunc)callback, 0);

  gtk_entry_set_text(entry, cur_item);

  for (i = 0; i < num_items; i ++)
      if (strcmp(items[i], cur_item) == 0)
	  break;

  if (i == num_items)
      gtk_entry_set_text(entry, gettext(items[0]));

  gtk_combo_set_use_arrows(GTK_COMBO(combo), TRUE);
  gtk_widget_set_sensitive(combo, TRUE);
  gtk_entry_set_editable(entry, FALSE);
  gtk_widget_show(combo);
}

/****************************************************************************
 *
 * gtk_do_misc_updates() - Build an option menu for the given parameters...
 *
 ****************************************************************************/
static void gtk_do_misc_updates(void)
{
  char s[255];
  stp_set_scaling(vars, stp_get_scaling(plist[plist_current].v)),
    stp_set_orientation(vars, stp_get_orientation(plist[plist_current].v));
  stp_set_left(vars, stp_get_left(plist[plist_current].v));
  stp_set_top(vars, stp_get_top(plist[plist_current].v));
  stp_set_unit(vars, stp_get_unit(plist[plist_current].v));

  gtk_preview_update();

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
      max_ppi_scaling = min_ppi_scaling * 20;
      if (tmp < min_ppi_scaling)
	{
	  tmp = min_ppi_scaling;
	  stp_set_scaling(vars, -tmp);
	}
      else if (tmp > max_ppi_scaling)
	{
	  tmp = max_ppi_scaling;
	  stp_set_scaling(vars, -tmp);
	}
      stp_set_scaling(plist[plist_current].v, tmp);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scaling_ppi), TRUE);
      GTK_ADJUSTMENT(scaling_adjustment)->lower = min_ppi_scaling;
      GTK_ADJUSTMENT(scaling_adjustment)->upper = max_ppi_scaling + 1;
      sprintf(s, "%.1f", tmp);
      GTK_ADJUSTMENT(scaling_adjustment)->value = tmp;
      gtk_signal_handler_block_by_data(GTK_OBJECT(scaling_entry), NULL);
      gtk_entry_set_text(GTK_ENTRY(scaling_entry), s);
      gtk_signal_handler_unblock_by_data(GTK_OBJECT(scaling_entry), NULL);
      gtk_signal_emit_by_name(scaling_adjustment, "value_changed");
      stp_set_scaling(plist[plist_current].v, stp_get_scaling(vars));
    }
  else
    {
      gdouble tmp = stp_get_scaling(plist[plist_current].v);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scaling_percent), TRUE);
      GTK_ADJUSTMENT(scaling_adjustment)->lower = 5.0;
      GTK_ADJUSTMENT(scaling_adjustment)->upper = 101.0;
      sprintf(s, "%.1f", tmp);
      GTK_ADJUSTMENT(scaling_adjustment)->value = tmp;
      gtk_signal_handler_block_by_data(GTK_OBJECT(scaling_entry), NULL);
      gtk_entry_set_text(GTK_ENTRY(scaling_entry), s);
      gtk_signal_handler_unblock_by_data(GTK_OBJECT(scaling_entry), NULL);
      gtk_signal_emit_by_name(scaling_adjustment, "value_changed");
    }

  gtk_do_color_updates();

  if (stp_get_output_type(plist[plist_current].v) == OUTPUT_GRAY)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(output_gray), TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(output_color), TRUE);

  if (stp_get_unit(plist[plist_current].v) == 0)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(unit_inch), TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(unit_cm), TRUE);

  switch (stp_get_image_type(plist[plist_current].v))
  {
    case IMAGE_LINE_ART:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(image_line_art), TRUE);
      break;
    case IMAGE_SOLID_TONE:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(image_solid_tone), TRUE);
      break;
    case IMAGE_CONTINUOUS:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(image_continuous_tone), TRUE);
      break;
    case IMAGE_MONOCHROME:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(image_monochrome), TRUE);
      break;
    default:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(image_continuous_tone), TRUE);
      stp_set_image_type(plist[plist_current].v, IMAGE_CONTINUOUS);
      break;
    }

  gtk_preview_update();
}

/****************************************************************************
 *
 * gtk_position_callback() - callback for position entry widgets
 *
 ****************************************************************************/
static void
gtk_position_callback (GtkWidget *widget)
{
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
	stp_set_top(vars, ((new_value + (1.0 / 144.0)) * 72) - (top + print_height));
      else if (widget == left_entry)
	stp_set_left(vars, ((new_value + (1.0 / 144.0)) * 72) - left);
      else if (widget == right_entry)
	stp_set_left(vars, ((new_value + (1.0 / 144.0)) * 72) - (left + print_width));
      else if (widget == width_entry)
	{
	  if (stp_get_scaling(vars) >= 0)
	    {
	      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (scaling_ppi),
					    TRUE);
	      gtk_scaling_callback (scaling_ppi);
	      was_percent = 1;
	    }
	  GTK_ADJUSTMENT(scaling_adjustment)->value =
	    ((gdouble) image_width) / new_value;
	  gtk_signal_emit_by_name (scaling_adjustment, "value_changed");
	  if (was_percent)
	    {
	      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (scaling_percent),
					    TRUE);
	      gtk_signal_emit_by_name (scaling_adjustment, "value_changed");
	    }
	}
      else if (widget == height_entry)
	{
	  if (stp_get_scaling(vars) >= 0)
	    {
	      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (scaling_ppi),
					    TRUE);
	      gtk_scaling_callback (scaling_ppi);
	      was_percent = 1;
	    }
	  GTK_ADJUSTMENT(scaling_adjustment)->value =
	    ((gdouble) image_height) / new_value;
	  gtk_signal_emit_by_name (scaling_adjustment, "value_changed");
	  if (was_percent)
	    {
	      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (scaling_percent),
					    TRUE);
	      gtk_signal_emit_by_name (scaling_adjustment, "value_changed");
	    }
	}
      if (stp_get_left(vars) < 0)
	stp_set_left(vars, 0);
      if (stp_get_top(vars) < 0)
	stp_set_top(vars, 0);
    }

  stp_set_left(plist[plist_current].v, stp_get_left(vars));
  stp_set_top(plist[plist_current].v, stp_get_top(vars));
  gtk_preview_update ();
}


/****************************************************************************
 *
 * gtk_plist_callback() - Update the current system printer...
 *
 ****************************************************************************/
static void gtk_plist_callback(GtkWidget *widget, /* I - Driver option menu */
			       gint      data)    /* I - Data */
{
  int		i;			/* Looping var */
  gp_plist_t	*p;
  int		num_media_sizes;
  char		**media_sizes;


  plist_current = data;
  p             = plist + plist_current;

  if (strcmp(stp_get_driver(p->v), ""))
    {
      stp_set_driver(vars, stp_get_driver(p->v));

      fprintf(stderr, "gtk_plist_callback current_printer was %x, ", current_printer);
      current_printer = stp_get_printer_by_driver(stp_get_driver(vars));
      fprintf(stderr, "now %x\n", current_printer);
    }

  stp_set_ppd_file(vars, stp_get_ppd_file(p->v));
  stp_set_media_size(vars, stp_get_media_size(p->v));
  stp_set_media_type(vars, stp_get_media_type(p->v));
  stp_set_media_source(vars, stp_get_media_source(p->v));
  stp_set_ink_type(vars, stp_get_ink_type(p->v));
  stp_set_dither_algorithm(vars, stp_get_dither_algorithm(p->v));
  stp_set_resolution(vars, stp_get_resolution(p->v));
  stp_set_output_to(vars, stp_get_output_to(p->v));
  gtk_do_misc_updates();

 /*
  * Now get option parameters...
  */
  gtk_build_dither_menu();

  media_sizes = (*(stp_printer_get_printfuncs(current_printer)->parameters))
    (current_printer, stp_get_ppd_file(p->v), "PageSize", &num_media_sizes);
  if (stp_get_media_size(vars)[0] == '\0')
    stp_set_media_size(vars, media_sizes[0]);

  gtk_plist_build_combo(media_size_combo,
		        num_media_sizes,
		        media_sizes,
		        (char *) stp_get_media_size(p->v),
		        gtk_media_size_callback);

  for (i = 0; i < num_media_sizes; i ++)
    free(media_sizes[i]);
  free(media_sizes);

  media_types = (*(stp_printer_get_printfuncs(current_printer)->parameters)) (current_printer,
						  stp_get_ppd_file(p->v),
						  "MediaType",
						  &num_media_types);
  if (stp_get_media_type(vars)[0] == '\0' && media_types != NULL)
    stp_set_media_type(vars, media_types[0]);
  else if (media_types == NULL)
    stp_set_media_type(vars, NULL);
  if (num_media_types > 0)
  gtk_plist_build_menu(media_type,
		       &media_type_menu,
		       num_media_types,
		       media_types,
		       (char *) stp_get_media_type(p->v),
		       gtk_media_type_callback);
  {
    for (i = 0; i < num_media_types; i ++)
      free(media_types[i]);
    free(media_types);
  }

  media_sources = (*(stp_printer_get_printfuncs(current_printer)->parameters)) (current_printer,
						    stp_get_ppd_file(p->v),
						    "InputSlot",
						    &num_media_sources);
  if (num_media_sources > 0)
  {
    for (i = 0; i < num_media_sources; i ++)
      free(media_sources[i]);
    free(media_sources);
  }

  if (stp_get_media_source(vars)[0] == '\0' && media_sources != NULL)
    stp_set_media_source(vars, media_sources[0]);
  else if (media_sources == NULL)
    stp_set_media_source(vars, NULL);
  gtk_plist_build_menu(media_source,
		       &media_source_menu,
		       num_media_sources,
		       media_sources,
		       (char *) stp_get_media_source(p->v),
		       gtk_media_source_callback);


  if (num_ink_types > 0)
  {
    for (i = 0; i < num_ink_types; i ++)
      free(ink_types[i]);
    free(ink_types);
  }

  ink_types = (*(stp_printer_get_printfuncs(current_printer)->parameters)) (current_printer,
						stp_get_ppd_file(p->v),
						"InkType", &num_ink_types);
  if (stp_get_ink_type(vars)[0] == '\0' && ink_types != NULL)
    stp_set_ink_type(vars, ink_types[0]);
  else if (ink_types == NULL)
    stp_set_ink_type(vars, NULL);
  gtk_plist_build_menu(ink_type,
		       &ink_type_menu,
		       num_ink_types,
		       ink_types,
		       (char *) stp_get_ink_type(p->v),
		       gtk_ink_type_callback);

  if (num_resolutions > 0)
  {
    for (i = 0; i < num_resolutions; i ++)
      free(resolutions[i]);
    free(resolutions);
  }

  resolutions = (*(stp_printer_get_printfuncs(current_printer)->parameters)) (current_printer,
						  stp_get_ppd_file(p->v),
						  "Resolution",
						  &num_resolutions);
  if (stp_get_resolution(vars)[0] == '\0' && resolutions != NULL)
    stp_set_resolution(vars, resolutions[0]);
  else if (resolutions == NULL)
    stp_set_resolution(vars, NULL);
  gtk_plist_build_menu(resolution,
		       &resolution_menu,
		       num_resolutions,
		       resolutions,
		       (char *) stp_get_resolution(p->v),
		       gtk_resolution_callback);
  if (dither_algo_menu)
    gtk_build_dither_menu();

}

/****************************************************************************
 *
 * gtk_media_size_callback() - Update the current media size...
 *
 ****************************************************************************/
static void gtk_media_size_callback(GtkWidget *widget, /* I -Media size menu */
				    gint      data)    /* I - Data */
{
  const char *new_media_size;
  if (ignore_combo_callback)
    return;
  new_media_size
    = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(media_size_combo)->entry));
  if (strcmp(stp_get_media_size(vars), new_media_size) != 0)
    {
      stp_set_media_size(vars, new_media_size);
      stp_set_media_size(plist[plist_current].v,new_media_size);
      stp_set_left(vars, -1);
      stp_set_top(vars, -1);
      stp_set_left(plist[plist_current].v, stp_get_left(vars));
      stp_set_top(plist[plist_current].v, stp_get_top(vars));
    }
  gtk_preview_update();
}


/****************************************************************************
 *
 * gtk_media_type_callback() - Update the current media type...
 *
 ****************************************************************************/
static void gtk_media_type_callback(GtkWidget *widget, /* I- Media type menu */
				    gint      data)    /* I - Data */
{
  stp_set_media_type(vars, media_types[data]);
  stp_set_media_type(plist[plist_current].v,media_types[data]);
  gtk_preview_update();
}


/****************************************************************************
 *
 * gtk_media_source_callback() - Update the current media source...
 *
 ****************************************************************************/
static void gtk_media_source_callback(GtkWidget *widget, /* I-Media source */
				      gint      data)    /* I - Data */
{
  stp_set_media_source(vars, media_sources[data]);
  stp_set_media_source(plist[plist_current].v,media_sources[data]);
  gtk_preview_update();
}

/****************************************************************************
 *
 * gtk_ink_type_callback() - Update the current ink type...
 *
 ****************************************************************************/
static void gtk_ink_type_callback(GtkWidget *widget, /* I-Ink type menu */
				  gint      data)    /* I - Data */
{
  stp_set_ink_type(vars, ink_types[data]);
  stp_set_ink_type(plist[plist_current].v,ink_types[data]);
  gtk_preview_update();
}

/****************************************************************************
 *
 * gtk_resolution_callback() - Update the current resolution...
 *
 ****************************************************************************/
static void gtk_resolution_callback(GtkWidget *widget, /* I-Media size menu */
				    gint      data)    /* I - Data */
{
  stp_set_resolution(vars, resolutions[data]);
  stp_set_resolution(plist[plist_current].v,resolutions[data]);
  gtk_preview_update();
}


/****************************************************************************
 *
 * gtk_orientation_callback() - Update the current media size...
 *
 ****************************************************************************/
static void gtk_orientation_callback(GtkWidget *widget,
				     gint      data)
{
  if (stp_get_orientation(vars) != (gint) data)
    {
      stp_set_orientation(vars, data);
      stp_set_left(vars, -1);
      stp_set_top(vars, -1);
      stp_set_orientation(plist[plist_current].v, stp_get_orientation(vars));
      stp_set_left(plist[plist_current].v, stp_get_left(vars));
      stp_set_top(plist[plist_current].v, stp_get_top(vars));
    }
  gtk_preview_update();
}


/****************************************************************************
 *
 * gtk_output_type_callback() - Update the current output type...
 *
 ****************************************************************************/
static void gtk_output_type_callback(GtkWidget *widget,
				     gint      data)
{
  if (GTK_TOGGLE_BUTTON(widget)->active)
  {
    stp_set_output_type(vars, data);
    stp_set_output_type(plist[plist_current].v, data);
  }
  gtk_preview_update();
}

/****************************************************************************
 *
 * gtk_unit_callback() - Update the current Unit...
 *
 ****************************************************************************/
static void gtk_unit_callback(GtkWidget *widget,
				     gint      data)
{
  if (GTK_TOGGLE_BUTTON(widget)->active)
  {
    stp_set_unit(vars, data);
    stp_set_unit(plist[plist_current].v, data);
    gtk_preview_update();
  }
}

/****************************************************************************
 *
 * gtk_linear_callback() - Update the current linear gradient mode...
 *
 ****************************************************************************/
#ifdef DO_LINEAR
static void gtk_linear_callback(GtkWidget *widget, /* I - Output type button */
				gint      data)    /* I - Data */
{
  if (GTK_TOGGLE_BUTTON(widget)->active)
  {
    stp_set_linear(vars, data);
    stp_set_linear(plist[plist_current].v, data);
  }
  gtk_preview_update();
}
#endif

/****************************************************************************
 *
 * image_type_callback() - Update the current linear gradient mode...
 *
 ****************************************************************************/
static void gtk_image_type_callback(GtkWidget *widget,
				    gint      data)
{
  if (GTK_TOGGLE_BUTTON(widget)->active)
  {
    stp_set_image_type(vars, data);
    stp_set_image_type(plist[plist_current].v, data);
  }
  gtk_preview_update();
}

/****************************************************************************
 *
 * gtk_print_callback() - Start the print...
 *
 ****************************************************************************/
static void gtk_print_callback(void)
{
  if (plist_current > 0)
    {
      runme = TRUE;
      gtk_widget_destroy (gtk_color_adjust_dialog);
      gtk_widget_destroy (setup_dialog);
      gtk_widget_destroy(print_dialog);
    }
  else
    {
      gtk_widget_set_sensitive (gtk_color_adjust_dialog, FALSE);
      gtk_widget_set_sensitive (setup_dialog, FALSE);
      gtk_widget_show(file_browser);
    }
}

/****************************************************************************
 *
 * gtk_printandsave_callback() -
 *
 ****************************************************************************/
static void gtk_printandsave_callback(void)
{
  saveme = TRUE;
  if (plist_current > 0)
    {
      runme = TRUE;
      gtk_widget_destroy (gtk_color_adjust_dialog);
      gtk_widget_destroy (setup_dialog);
      gtk_widget_destroy(print_dialog);
    }
  else
    {
      gtk_widget_set_sensitive (gtk_color_adjust_dialog, FALSE);
      gtk_widget_set_sensitive (setup_dialog, FALSE);
      gtk_widget_show(file_browser);
    }
}

/****************************************************************************
 *
 * gtk_save_callback() - save settings, don't destroy dialog
 *
 ****************************************************************************/
static void gtk_save_callback(void)
{
  printrc_save();
  gtk_widget_grab_default(printandsave_button);
}

/****************************************************************************
 *
 * gtk_cancel_callback() - Cancel the print...
 *
 ****************************************************************************/
static void gtk_cancel_callback(void)
{
  gtk_widget_destroy(print_dialog);
}

/****************************************************************************
 *
 * gtk_close_callback() - Exit the print dialog application.
 *
 ****************************************************************************/
static void gtk_close_callback(void)
{
  gtk_main_quit();
}

/****************************************************************************
 *
 * gtk_setup_open__callback() -
 *
 ****************************************************************************/
static void gtk_setup_open_callback(void)
{
  GtkAdjustment *adjustment;
  int idx;

  current_printer = stp_get_printer_by_driver(stp_get_driver(plist[plist_current].v));
  idx = stp_get_printer_index_by_driver(stp_get_driver(plist[plist_current].v));

  gtk_clist_select_row(GTK_CLIST(printer_driver), idx, 0);

  gtk_entry_set_text(GTK_ENTRY(ppd_file), stp_get_ppd_file(plist[plist_current].v));

  if (strncmp(stp_get_driver(plist[plist_current].v) ,"ps", 2) == 0)
    {
      gtk_widget_show(ppd_file);
      gtk_widget_show(ppd_button);
    }
  else
    {
      gtk_widget_hide(ppd_file);
      gtk_widget_hide(ppd_button);
    }

  gtk_entry_set_text(GTK_ENTRY(output_cmd),
		     stp_get_output_to(plist[plist_current].v));

  if (plist_current == 0)
    gtk_widget_hide(output_cmd);
  else
    gtk_widget_show(output_cmd);

  gtk_widget_show (setup_dialog);
  adjustment =
    gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(printer_crawler));
  gtk_adjustment_set_value(adjustment, idx * (adjustment->step_increment + 3));
  gtk_widget_show(setup_dialog);
}

/****************************************************************************
 *
 * gtk_setup_ok_callback() -
 *
 ****************************************************************************/
static void gtk_setup_ok_callback(void)
{
  stp_set_driver(vars, stp_printer_get_driver(current_printer));
  stp_set_driver(plist[plist_current].v,stp_printer_get_driver(current_printer));

  stp_set_output_to(vars, gtk_entry_get_text(GTK_ENTRY(output_cmd)));
  stp_set_output_to(plist[plist_current].v,stp_get_output_to(vars));

  stp_set_ppd_file(vars, gtk_entry_get_text(GTK_ENTRY(ppd_file)));
  stp_set_ppd_file(plist[plist_current].v,stp_get_ppd_file(vars));

  gtk_plist_callback(NULL, plist_current);

  gtk_widget_hide(setup_dialog);
}

/****************************************************************************
 *
 * gtk_setup_cancel_callback() -
 *
 ****************************************************************************/
static void gtk_setup_cancel_callback(void)
{
  gtk_widget_hide(setup_dialog);
}

/****************************************************************************
 *
 * print_driver_callback() - Update the current printer driver...
 *
 ****************************************************************************/
static void gtk_print_driver_callback(GtkWidget *widget, /* I - Driver list */
					  gint		row,
					  gint		column,
					  GdkEventButton	*event,
				      gpointer      data)    /* I - Data */
{
  data = gtk_clist_get_row_data(GTK_CLIST(widget), row);
  current_printer = stp_get_printer_by_index((int) data);

  if (strncmp(stp_printer_get_driver(current_printer), "ps", 2) == 0)
  {
    gtk_widget_show(ppd_file);
    gtk_widget_show(ppd_button);
  }
  else
  {
    gtk_widget_hide(ppd_file);
    gtk_widget_hide(ppd_button);
  }
}

/****************************************************************************
 *
 * gtk_ppd_browse_callback() -
 *
 ****************************************************************************/
static void gtk_ppd_browse_callback(void)
{
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(ppd_browser),
                                  gtk_entry_get_text(GTK_ENTRY(ppd_file)));
  gtk_widget_show(ppd_browser);
}

/****************************************************************************
 *
 * gtk_ppd_ok_callback() -
 *
 ****************************************************************************/
static void gtk_ppd_ok_callback(void)
{
  gtk_widget_hide(ppd_browser);
  gtk_entry_set_text(GTK_ENTRY(ppd_file),
      gtk_file_selection_get_filename(GTK_FILE_SELECTION(ppd_browser)));
}

/****************************************************************************
 *
 * gtk_ppd_cancel_callback() -
 *
 ****************************************************************************/
static void gtk_ppd_cancel_callback(void)
{
  gtk_widget_hide(ppd_browser);
}

/****************************************************************************
 *
 * gtk_file_ok_callback() - print to file and go away
 *
 ****************************************************************************/
static void gtk_file_ok_callback(void)
{
  gtk_widget_hide(file_browser);
  stp_set_output_to(vars,
		    gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_browser)));

  runme = TRUE;
  gtk_widget_destroy (gtk_color_adjust_dialog);
  gtk_widget_destroy (setup_dialog);
  gtk_widget_destroy(print_dialog);
}

/****************************************************************************
 *
 * gtk_file_cancel_callback() -
 *
 ****************************************************************************/
static void gtk_file_cancel_callback(void)
{
  gtk_widget_hide(file_browser);
  gtk_widget_set_sensitive (gtk_color_adjust_dialog, TRUE);
  gtk_widget_set_sensitive (setup_dialog, TRUE);
  gtk_widget_set_sensitive (print_dialog, TRUE);
}

/****************************************************************************
 *
 * gtk_preview_update_callback() -
 *
 ****************************************************************************/
static void gtk_preview_update(void)
{
  int		temp,		/* Swapping variable */
		orient;		/* True orientation of printable */
  gdouble	max_ppi_scaling;   /* Maximum PPI for current page size */
  gdouble	min_ppi_scaling;/* Minimum PPI for current page size */
  gdouble	min_ppi_scaling1;   /* Minimum PPI for current page size */
  gdouble	min_ppi_scaling2;   /* Minimum PPI for current page size */
  int           paper_left, paper_top;
  static GdkGC	*gc = NULL,	/* Normal graphics context */
		*gcinv = NULL;	/* GC for inverted drawing (arrow) */
  char s[255];
  gdouble unit_scaler;


  (*stp_printer_get_printfuncs(current_printer)->media_size)
    (current_printer, vars, &paper_width, &paper_height);

  (*stp_printer_get_printfuncs(current_printer)->imageable_area)
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
    right             = temp;
    temp              = top;
    top               = bottom;
    bottom            = temp;
  }

  bottom = paper_height - bottom;
  right = paper_width - right;

  if (stp_get_scaling(vars) < 0)
  {
    double twidth;
    min_ppi_scaling1 = 72.0 * (gdouble) image_width / printable_width;
    min_ppi_scaling2 = 72.0 * (gdouble) image_height / printable_height;
    if (min_ppi_scaling1 > min_ppi_scaling2)
      min_ppi_scaling = min_ppi_scaling1;
    else
      min_ppi_scaling = min_ppi_scaling2;
    max_ppi_scaling = min_ppi_scaling * 20;
    if (stp_get_scaling(vars) > -min_ppi_scaling)
      stp_set_scaling(vars, -min_ppi_scaling);
    twidth = (72.0 * image_width / -stp_get_scaling(vars));
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
      double twidth = .5 + printable_width * stp_get_scaling(vars) / 100;
      print_width = twidth;
      print_height = twidth * image_height / image_width;
    }
    else
    {
      double theight = .5 + printable_height * stp_get_scaling(vars) / 100;
      print_height = theight;
      print_width = theight * image_width / image_height;
    }
  }

  if (paper_width == 0)
    paper_width = 1;
  if (paper_height == 0)
    paper_height = 1;

  preview_ppi = PREVIEW_SIZE_HORIZ * 72 / paper_width;
  if (PREVIEW_SIZE_VERT * 72 / paper_height < preview_ppi)
    preview_ppi = PREVIEW_SIZE_VERT * 72 / paper_height;
  if (preview_ppi > MAX_PREVIEW_PPI)
    preview_ppi = MAX_PREVIEW_PPI;

  paper_left = (PREVIEW_SIZE_HORIZ - preview_ppi * paper_width / 72) / 2;
  paper_top  = (PREVIEW_SIZE_VERT - preview_ppi * paper_height / 72) / 2;
  printable_left = paper_left +  preview_ppi * left / 72;
  printable_top  = paper_top + preview_ppi * top / 72 ;

  if (preview->widget.window == NULL)
    return;

  gdk_window_clear(preview->widget.window);

  if (gc == NULL)
  {
    gc = gdk_gc_new(preview->widget.window);
    gcinv = gdk_gc_new(preview->widget.window);
    gdk_gc_set_function (gcinv, GDK_INVERT);
  }

  /* draw paper frame */

  gdk_draw_rectangle(preview->widget.window, gc, 0,
		     paper_left, paper_top,
                     preview_ppi * paper_width / 72,
                     preview_ppi * paper_height / 72);

  /* draw printable frame */
  gdk_draw_rectangle(preview->widget.window, gc, 0,
                     printable_left, printable_top,
                     preview_ppi * printable_width / 72,
                     preview_ppi * printable_height / 72);


  if (stp_get_left(vars) < 0)		/* centre */
    stp_set_left(vars, ((paper_width - print_width) / 2) - left);

  /* shall we really force to printable area? for now we leave it */
  /* we leave stp_get_left(vars) etc. relative to printable area */

  if (stp_get_left(vars) > (printable_width - print_width))
      stp_set_left(vars, printable_width - print_width);

  if (stp_get_top(vars) < 0)
    stp_set_top(vars, ((paper_height - print_height) / 2) - top);

  if (stp_get_top(vars) > (printable_height - print_height))
      stp_set_top(vars, printable_height - print_height);

  stp_set_left(plist[plist_current].v, stp_get_left(vars));
  stp_set_top(plist[plist_current].v, stp_get_top(vars));


  if(stp_get_unit(vars)) unit_scaler = 72.0/2.54;
  else unit_scaler = 72.0;
  sprintf(s, "%.2f", (top + stp_get_top(vars)) / unit_scaler);
  gtk_signal_handler_block_by_data(GTK_OBJECT(top_entry), NULL);
  gtk_entry_set_text(GTK_ENTRY(top_entry), s);
  gtk_signal_handler_unblock_by_data(GTK_OBJECT(top_entry), NULL);

  sprintf(s, "%.2f", (left + stp_get_left(vars)) / unit_scaler);
  gtk_signal_handler_block_by_data(GTK_OBJECT(left_entry), NULL);
  gtk_entry_set_text(GTK_ENTRY(left_entry), s);
  gtk_signal_handler_unblock_by_data(GTK_OBJECT(left_entry), NULL);

  gtk_signal_handler_block_by_data(GTK_OBJECT(bottom_entry), NULL);
  sprintf(s, "%.2f", ((top + stp_get_top(vars)) + print_height) / unit_scaler);
  gtk_entry_set_text(GTK_ENTRY(bottom_entry), s);
  gtk_signal_handler_unblock_by_data(GTK_OBJECT(bottom_entry), NULL);

  gtk_signal_handler_block_by_data(GTK_OBJECT(right_entry), NULL);
  sprintf(s, "%.2f", (left + stp_get_left(vars) + print_width) / unit_scaler);
  gtk_entry_set_text(GTK_ENTRY(right_entry), s);
  gtk_signal_handler_unblock_by_data(GTK_OBJECT(right_entry), NULL);

  gtk_signal_handler_block_by_data(GTK_OBJECT(width_entry), NULL);
  sprintf(s, "%.2f", print_width / unit_scaler);
  gtk_entry_set_text(GTK_ENTRY(width_entry), s);
  gtk_signal_handler_unblock_by_data(GTK_OBJECT(width_entry), NULL);

  gtk_signal_handler_block_by_data(GTK_OBJECT(height_entry), NULL);
  sprintf(s, "%.2f", print_height / unit_scaler);
  gtk_entry_set_text(GTK_ENTRY(height_entry), s);
  gtk_signal_handler_unblock_by_data(GTK_OBJECT(height_entry), NULL);

  /* draw image  */
  gdk_draw_rectangle(preview->widget.window, gc, 1,
                     1 + printable_left + preview_ppi * stp_get_left(vars) / 72,
                     1 + printable_top + preview_ppi * stp_get_top(vars) / 72,
                     (preview_ppi * print_width + 71) / 72,
                     (preview_ppi * print_height + 71) / 72);

  /* draw orientation arrow pointing to top-of-paper */
  {
    int ox, oy, u;
    u = preview_ppi/2;
    ox = paper_left + preview_ppi * paper_width / 72 / 2;
    oy = paper_top + preview_ppi * paper_height / 72 / 2;
    if (orient == ORIENT_LANDSCAPE) {
      ox += preview_ppi * paper_width / 72 / 4;
      if (ox > paper_left + preview_ppi * paper_width / 72 - u)
        ox = paper_left + preview_ppi * paper_width / 72 - u;
      gdk_draw_line (preview->widget.window, gcinv, ox + u, oy, ox, oy - u);
      gdk_draw_line (preview->widget.window, gcinv, ox + u, oy, ox, oy + u);
      gdk_draw_line (preview->widget.window, gcinv, ox + u, oy, ox - u, oy);
    } else if (orient == ORIENT_SEASCAPE) {
      ox -= preview_ppi * paper_width / 72 / 4;
      if (ox < paper_left + u)
        ox = paper_left + u;
      gdk_draw_line (preview->widget.window, gcinv, ox - u, oy, ox, oy - u);
      gdk_draw_line (preview->widget.window, gcinv, ox - u, oy, ox, oy + u);
      gdk_draw_line (preview->widget.window, gcinv, ox - u, oy, ox + u, oy);
    } else if (orient == ORIENT_UPSIDEDOWN) {
      oy += preview_ppi * paper_height / 72 / 4;
      if (oy > paper_top + preview_ppi * paper_height / 72 - u)
        oy = paper_top + preview_ppi * paper_height / 72 - u;
      gdk_draw_line (preview->widget.window, gcinv, ox, oy + u, ox - u, oy);
      gdk_draw_line (preview->widget.window, gcinv, ox, oy + u, ox + u, oy);
      gdk_draw_line (preview->widget.window, gcinv, ox, oy + u, ox, oy - u);
    } else /* (orient == ORIENT_PORTRAIT) */ {
      oy -= preview_ppi * paper_height / 72 / 4;
      if (oy < paper_top + u)
        oy = paper_top + u;
      gdk_draw_line (preview->widget.window, gcinv, ox, oy - u, ox - u, oy);
      gdk_draw_line (preview->widget.window, gcinv, ox, oy - u, ox + u, oy);
      gdk_draw_line (preview->widget.window, gcinv, ox, oy - u, ox, oy + u);
    }
  }

  gdk_flush();

}

/****************************************************************************
 *
 * gtk_preview_button_callback() -
 *
 ****************************************************************************/
static void gtk_preview_button_callback(GtkWidget      *w,
					GdkEventButton *event)
{
  mouse_x = event->x;
  mouse_y = event->y;
  mouse_button = event->button;
}

/****************************************************************************
 *
 * gtk_preview_motion_callback() -
 *
 ****************************************************************************/
static void gtk_preview_motion_callback(GtkWidget      *w,
					GdkEventMotion *event)
{

  if (stp_get_left(vars) < 0 || stp_get_top(vars) < 0)
    {
      stp_set_left(vars, 72 * (printable_width - print_width) / 20);
      stp_set_top(vars, 72 * (printable_height - print_height) / 20);
    }

  if (mouse_button == 1)
    {
      stp_set_left(vars, stp_get_left(vars) + 72 * (event->x - mouse_x) / preview_ppi);
      stp_set_top(vars, stp_get_top(vars) + 72 * (event->y - mouse_y) / preview_ppi);
    }
  else
    {
      stp_set_left(vars, stp_get_left(vars) + event->x - mouse_x);
      stp_set_top(vars, stp_get_top(vars) + event->y - mouse_y);
    }

  if (stp_get_left(vars) < 0)
    stp_set_left(vars, 0);

  if (stp_get_top(vars) < 0)
    stp_set_top(vars, 0);
  stp_set_left(plist[plist_current].v, stp_get_left(vars));
  stp_set_top(plist[plist_current].v, stp_get_top(vars));

  gtk_preview_update();

  mouse_x = event->x;
  mouse_y = event->y;
}

/****************************************************************************
 *
 * gtk_setup_open__callback() -
 *
 ****************************************************************************/
static void gtk_show_adjust_button_callback(GtkWidget * w)
{
    gtk_widget_show(gtk_color_adjust_dialog);
}

#endif /* ! NEW_UI_ONLY */
