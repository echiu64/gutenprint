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
 *
 * Contents:
 *
 * create_main_window()                   - Create main window for pug-in  
 *
 * Revision History:
 *
 *   See ChangeLog
 */

#include "print.h"
#include "print_gimp.h"

#ifdef GIMP_1_0

#define N_(x) x
#define _(x) x
#define gettext(x) x

/*
 * Constants for GUI...
 */


extern vars_t vars;
extern int plist_count;	     /* Number of system printers */
extern int plist_current;    /* Current system printer */
extern plist_t  plist[MAX_PLIST];       /* System printers */
extern gint32 image_ID;
extern int image_width;
extern int image_height;
extern const printer_t *current_printer;
extern int runme;
extern int saveme;
extern GtkWidget* gtk_color_adjust_dialog;

void  printrc_save(void);

/***
 * Main window widgets
 ***/
static GtkWidget* print_dialog;           /* Print dialog window */
static GtkWidget* recenter_button;
static GtkWidget* left_entry;
static GtkWidget* right_entry;
static GtkWidget* top_entry;
static GtkWidget* bottom_entry;
static GtkWidget* media_size;             /* Media size option button */
static GtkWidget* media_size_menu=NULL;   /* Media size menu */
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
#ifndef GIMP_1_0
static GtkWidget* scaling_image;          /* Scale to the image */
#endif
static GtkWidget* output_gray;            /* Output type toggle, black */
static GtkWidget* output_color;           /* Output type toggle, color */
#ifdef DO_LINEAR
static GtkWidget* linear_on;              /* Linear toggle, on */
static GtkWidget* linear_off;             /* Linear toggle, off */
#endif
static GtkWidget* image_line_art;
static GtkWidget* image_solid_tone;
static GtkWidget* image_continuous_tone;
static GtkWidget* image_monochrome;
static GtkWidget* setup_dialog;           /* Setup dialog window */
static GtkWidget* printer_driver;         /* Printer driver widget */
static GtkWidget* ppd_file;               /* PPD file entry */
static GtkWidget* ppd_button;             /* PPD file browse button */
static GtkWidget* output_cmd;             /* Output command text entry */
static GtkWidget* ppd_browser;            /* File selection dialog for PPD files */
static GtkWidget* file_browser;           /* FSD for print files */
static GtkWidget* printandsave_button;
static GtkWidget* adjust_color_button;

static GtkObject* scaling_adjustment;	   /* Adjustment object for scaling */

static int		num_media_sizes=0;	/* Number of media sizes */
static char		**media_sizes;		/* Media size strings */
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
	        	mouse_y;		/* Last mouse Y */

static int 		page_left;		/* Left pixel column of page */
static int		page_top;		/* Top pixel row of page */
static int		page_width;		/* Width of page on screen */
static int		page_height;		/* Height of page on screen */
static int		print_width;		/* Printed width of image */
static int		print_height;		/* Printed height of image */

extern GtkObject* brightness_adjustment; /* Adjustment object for brightness */
extern GtkObject* saturation_adjustment; /* Adjustment object for saturation */
extern GtkObject* density_adjustment;	 /* Adjustment object for density */
extern GtkObject* contrast_adjustment;	 /* Adjustment object for contrast */
extern GtkObject* red_adjustment;	 /* Adjustment object for red */
extern GtkObject* green_adjustment;	 /* Adjustment object for green */
extern GtkObject* blue_adjustment;	 /* Adjustment object for blue */
extern GtkObject* gamma_adjustment;	 /* Adjustment object for gamma */

static void gtk_scaling_update(GtkAdjustment *);
static void gtk_scaling_callback(GtkWidget *);
static void gtk_plist_callback(GtkWidget *, gint);
static void gtk_media_size_callback(GtkWidget *, gint);
static void gtk_media_type_callback(GtkWidget *, gint);
static void gtk_media_source_callback(GtkWidget *, gint);
static void gtk_ink_type_callback(GtkWidget *, gint);
static void gtk_resolution_callback(GtkWidget *, gint);
static void gtk_output_type_callback(GtkWidget *, gint);
#ifdef DO_LINEAR
static void gtk_linear_callback(GtkWidget *, gint);
#endif
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
static void gtk_print_driver_callback(GtkWidget *, gint);

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

/*****************************************************************************
 *
 * gtk_create_main_window()
 *
 * NOTES:
 *   
 *****************************************************************************/
void gtk_create_main_window(void)
{
    int        i;          /* Looping var */
    char       s[100];     /* Text string */
    GtkWidget* label;      /* Label string */
    GtkWidget* hbbox;      /* button_box for OK/Cancel buttons */
    GtkWidget* button;     /* OK/Cancel buttons */
    GtkWidget* scale;      /* Scale widget */
    GtkWidget* entry;      /* Text entry widget */
    GtkWidget* menu;       /* Menu of drivers/sizes */
    GtkWidget* item;       /* Menu item */
    GtkWidget* option;     /* Option menu button */
    GtkWidget* box;        /* Box container */
    GtkObject* scale_data; /* Scale data (limits) */
    GSList*    group;      /* Grouping for output type */
#ifdef DO_LINEAR
    GSList* linear_group;  /* Grouping for linear scale */
#endif
    GSList*    image_type_group;  /* Grouping for image type */

    const printer_t *the_printer = get_printer_by_index(0);

    static char   *orients[] =    /* Orientation strings */
    {
	N_("Auto"),
	N_("Portrait"),
	N_("Landscape")
    };
    gchar *plug_in_name;

    /***
     * Create the main dialog
     ***/
    print_dialog = dialog = gtk_dialog_new();

    plug_in_name = g_strdup_printf (_("Print v%s"), PLUG_IN_VERSION);
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
     * Drawing area for page preview...
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
    sprintf(s, "%.3f", fabs(vars.left));
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
    sprintf(s, "%.3f", fabs(vars.top));
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
    sprintf(s, "%.3f", fabs(vars.left));
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
    sprintf(s, "%.3f", fabs(vars.left));
    gtk_entry_set_text(GTK_ENTRY(entry), s);
    gtk_signal_connect(GTK_OBJECT(entry), "activate",
		       (GtkSignalFunc)gtk_position_callback, NULL);
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
    gtk_widget_set_usize(entry, 60, 0);
    gtk_widget_show(entry);

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

    media_size = option = gtk_option_menu_new();
    gtk_box_pack_start(GTK_BOX(box), option, FALSE, FALSE, 0);

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
    gtk_option_menu_set_history(GTK_OPTION_MENU(option), vars.orientation + 1);
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
    if (vars.output_type == 0)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_signal_connect(GTK_OBJECT(button), "toggled",
		       (GtkSignalFunc)gtk_output_type_callback,
		       (gpointer)OUTPUT_GRAY);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    output_color = button = gtk_radio_button_new_with_label(group, _("Color"));
    if (vars.output_type == 1)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_signal_connect(GTK_OBJECT(button), "toggled",
		       (GtkSignalFunc)gtk_output_type_callback,
		       (gpointer)OUTPUT_COLOR);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

#ifdef DO_LINEAR
    label = gtk_label_new(_("Output Level:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label,
		     3, 4,
		     10, 11,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(label);

    box = gtk_hbox_new(FALSE, 8);
    gtk_table_attach(GTK_TABLE(table),
		     box,
		     4, 5,
		     10, 11,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(box);

    linear_off = button =
	              gtk_radio_button_new_with_label(NULL, _("Normal scale"));
    linear_group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
    if (vars.linear == 0)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_signal_connect(GTK_OBJECT(button), "toggled",
		       (GtkSignalFunc)gtk_linear_callback,
		       (gpointer)0);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    linear_on = button = gtk_radio_button_new_with_label(linear_group,
					       _("Experimental linear scale"));
    if (vars.linear == 1)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_signal_connect(GTK_OBJECT(button), "toggled",
		       (GtkSignalFunc)gtk_linear_callback,
		       (gpointer)1);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);
#endif

    /*
     * Image type
     */
    label = gtk_label_new(_("Image Type:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label,
		     0, 1,
		     10, 11,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(label);

    box = gtk_hbox_new(FALSE, 8);
    gtk_table_attach(GTK_TABLE(table),
		     box,
		     1, 4,
		     10, 11,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(box);

    image_line_art= button =
	gtk_radio_button_new_with_label(NULL, _("Line Art"));
    image_type_group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
    if (vars.image_type == IMAGE_LINE_ART)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_signal_connect(GTK_OBJECT(button), "toggled",
		       (GtkSignalFunc)gtk_image_type_callback,
		       (gpointer)IMAGE_LINE_ART);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    image_solid_tone= button =
	gtk_radio_button_new_with_label(image_type_group, _("Solid Colors"));
    if (vars.image_type == IMAGE_SOLID_TONE)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_signal_connect(GTK_OBJECT(button), "toggled",
		       (GtkSignalFunc)gtk_image_type_callback,
		       (gpointer)IMAGE_SOLID_TONE);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    image_continuous_tone= button =
	gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(button)),
					_("Photograph"));
    if (vars.image_type == IMAGE_CONTINUOUS)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_signal_connect(GTK_OBJECT(button), "toggled",
		       (GtkSignalFunc)gtk_image_type_callback,
		       (gpointer)IMAGE_CONTINUOUS);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    image_monochrome= button =
	gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(button)),
					_("Monochrome"));
    if (vars.image_type == IMAGE_MONOCHROME)
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
    
    if (vars.scaling < 0.0)
	scaling_adjustment = scale_data =
	    gtk_adjustment_new(-vars.scaling, 36.0, 1201.0, 1.0, 1.0, 1.0);
    else
	scaling_adjustment = scale_data =
	    gtk_adjustment_new(vars.scaling, 5.0, 101.0, 1.0, 1.0, 1.0);

    gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
		       (GtkSignalFunc)gtk_scaling_update, NULL);

    scaling_scale = scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
    gtk_box_pack_start(GTK_BOX(box), scale, FALSE, FALSE, 0);
    gtk_widget_set_usize(scale, 200, 0);
    gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
    gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
    gtk_widget_show(scale);

    scaling_entry = entry = gtk_entry_new();
    sprintf(s, "%.1f", fabs(vars.scaling));
    gtk_entry_set_text(GTK_ENTRY(entry), s);
    gtk_signal_connect(GTK_OBJECT(entry), "changed",
		       (GtkSignalFunc)gtk_scaling_callback, NULL);
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
    gtk_widget_set_usize(entry, 60, 0);
    gtk_widget_show(entry);

    scaling_percent = button = gtk_radio_button_new_with_label(NULL,
							       _("Percent"));
    group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
    if (vars.scaling > 0.0)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    scaling_ppi = button = gtk_radio_button_new_with_label(group, _("PPI"));
    if (vars.scaling < 0.0)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    gtk_signal_connect(GTK_OBJECT(scaling_percent), "toggled",
		       (GtkSignalFunc)gtk_scaling_callback, NULL);
    gtk_signal_connect(GTK_OBJECT(scaling_ppi), "toggled",
		       (GtkSignalFunc)gtk_scaling_callback, NULL);

#ifndef GIMP_1_0
    scaling_image = button = gtk_button_new_with_label(_("Set Image Scale"));
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc)gtk_scaling_callback, NULL);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_widget_show(button);
#endif

    /***
     *  Color adjust button
     ***/
    box = gtk_hbox_new(FALSE, 8);
    gtk_table_attach(GTK_TABLE(table),
		     box,
		     3, 4, 12, 13,
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
    table = gtk_table_new(3, 2, FALSE);
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
     * Printer driver option menu...
     */
    label = gtk_label_new(_("Driver:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label,
		     0, 1,
		     0, 1,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(label);

    menu = gtk_menu_new();
    for (i = 0; i < known_printers(); i ++)
    {
	if (!strcmp(the_printer->long_name, ""))
	    continue;
	item = gtk_menu_item_new_with_label(gettext(the_printer->long_name));
	gtk_menu_append(GTK_MENU(menu), item);
	gtk_signal_connect(GTK_OBJECT(item), "activate",
			   (GtkSignalFunc)gtk_print_driver_callback,
			   (gpointer)i);
	gtk_widget_show(item);
	the_printer++;
    }

    printer_driver = option = gtk_option_menu_new();
    gtk_table_attach(GTK_TABLE(table),
		     option,
		     1, 2,
		     0, 1,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(option), menu);
    gtk_widget_show(option);

    /*
     * PPD file...
     */
    label = gtk_label_new(_("PPD File:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(GTK_TABLE(table),
		     label,
		     0, 1,
		     1, 2,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(label);

    box = gtk_hbox_new(FALSE, 8);
    gtk_table_attach(GTK_TABLE(table),
		     box,
		     1, 2,
		     1, 2,
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
		     2, 3,
		     GTK_FILL, GTK_FILL,
		     0, 0);
    gtk_widget_show(label);
    
    output_cmd = entry = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table),
		     entry,
		     1, 2,
		     2, 3,
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


  if (vars.scaling != adjustment->value)
  {
    if (GTK_TOGGLE_BUTTON(scaling_ppi)->active)
      vars.scaling = -adjustment->value;
    else
      vars.scaling = adjustment->value;
    plist[plist_current].v.scaling = vars.scaling;

    sprintf(s, "%.1f", adjustment->value);

    gtk_signal_handler_block_by_data(GTK_OBJECT(scaling_entry), NULL);
    gtk_entry_set_text(GTK_ENTRY(scaling_entry), s);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(scaling_entry), NULL);

    gtk_preview_update();
  }
}


/****************************************************************************
 *
 * gtk_scaling_callback() - Update the scaling field using the text entry
 *
 ****************************************************************************/
static void gtk_scaling_callback(GtkWidget* widget) /* I - New value */
{
  gfloat	new_value;		/* New scaling value */


  if (widget == scaling_entry)
  {
    new_value = atof(gtk_entry_get_text(GTK_ENTRY(widget)));

    if (vars.scaling != new_value)
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
    GTK_ADJUSTMENT(scaling_adjustment)->lower = 36.0;
    GTK_ADJUSTMENT(scaling_adjustment)->upper = 1201.0;
    GTK_ADJUSTMENT(scaling_adjustment)->value = 72.0;
    vars.scaling = 0.0;
    plist[plist_current].v.scaling = vars.scaling;
    gtk_signal_emit_by_name(scaling_adjustment, "value_changed");
  }
  else if (widget == scaling_percent)
  {
    GTK_ADJUSTMENT(scaling_adjustment)->lower = 5.0;
    GTK_ADJUSTMENT(scaling_adjustment)->upper = 101.0;
    GTK_ADJUSTMENT(scaling_adjustment)->value = 100.0;
    vars.scaling = 0.0;
    plist[plist_current].v.scaling = vars.scaling;
    gtk_signal_emit_by_name(scaling_adjustment, "value_changed");
  }
#ifndef GIMP_1_0
  else if (widget == scaling_image)
  {
    double xres, yres;
    gimp_image_get_resolution(image_ID, &xres, &yres);
    GTK_ADJUSTMENT(scaling_adjustment)->lower = 36.0;
    GTK_ADJUSTMENT(scaling_adjustment)->upper = 1201.0;
    GTK_ADJUSTMENT(scaling_adjustment)->value = yres;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scaling_ppi), TRUE);
    vars.scaling = 0.0;
    plist[plist_current].v.scaling = vars.scaling;
    gtk_signal_emit_by_name(scaling_adjustment, "value_changed");
  }
#endif
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
 * gtk_do_misc_updates() - Build an option menu for the given parameters...
 *
 ****************************************************************************/
static void gtk_do_misc_updates()
{
  char s[255];
  vars.scaling = plist[plist_current].v.scaling;
  vars.orientation = plist[plist_current].v.orientation;
  vars.left = plist[plist_current].v.left;
  vars.top = plist[plist_current].v.top;

  if (plist[plist_current].v.scaling < 0)
    {
      float tmp = -plist[plist_current].v.scaling;
      plist[plist_current].v.scaling = -plist[plist_current].v.scaling;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scaling_ppi), TRUE);
      GTK_ADJUSTMENT(scaling_adjustment)->lower = 36.0;
      GTK_ADJUSTMENT(scaling_adjustment)->upper = 1201.0;
      sprintf(s, "%.1f", tmp);
      GTK_ADJUSTMENT(scaling_adjustment)->value = tmp;
      gtk_signal_handler_block_by_data(GTK_OBJECT(scaling_entry), NULL);
      gtk_entry_set_text(GTK_ENTRY(scaling_entry), s);
      gtk_signal_handler_unblock_by_data(GTK_OBJECT(scaling_entry), NULL);
      gtk_signal_emit_by_name(scaling_adjustment, "value_changed");
    }
  else
    {
      float tmp = plist[plist_current].v.scaling;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scaling_percent), TRUE);
      GTK_ADJUSTMENT(scaling_adjustment)->lower = 5.0;
      GTK_ADJUSTMENT(scaling_adjustment)->upper = 101.0;
      sprintf(s, "%.1f", tmp);
      GTK_ADJUSTMENT(scaling_adjustment)->value = tmp;
      gtk_signal_handler_block_by_data(GTK_OBJECT(scaling_entry), NULL);
      gtk_entry_set_text(GTK_ENTRY(scaling_entry), s);
      gtk_signal_handler_unblock_by_data(GTK_OBJECT(scaling_entry), NULL);
      gtk_signal_emit_by_name(scaling_adjustment, "value_changed");
    }    

  GTK_ADJUSTMENT(brightness_adjustment)->value = plist[plist_current].v.brightness;
  gtk_signal_emit_by_name(brightness_adjustment, "value_changed");

  GTK_ADJUSTMENT(gamma_adjustment)->value = plist[plist_current].v.gamma;
  gtk_signal_emit_by_name(gamma_adjustment, "value_changed");

  GTK_ADJUSTMENT(contrast_adjustment)->value = plist[plist_current].v.contrast;
  gtk_signal_emit_by_name(contrast_adjustment, "value_changed");

  GTK_ADJUSTMENT(red_adjustment)->value = plist[plist_current].v.red;
  gtk_signal_emit_by_name(red_adjustment, "value_changed");

  GTK_ADJUSTMENT(green_adjustment)->value = plist[plist_current].v.green;
  gtk_signal_emit_by_name(green_adjustment, "value_changed");

  GTK_ADJUSTMENT(blue_adjustment)->value = plist[plist_current].v.blue;
  gtk_signal_emit_by_name(blue_adjustment, "value_changed");

  GTK_ADJUSTMENT(saturation_adjustment)->value = plist[plist_current].v.saturation;
  gtk_signal_emit_by_name(saturation_adjustment, "value_changed");

  GTK_ADJUSTMENT(density_adjustment)->value = plist[plist_current].v.density;
  gtk_signal_emit_by_name(density_adjustment, "value_changed");

  if (plist[plist_current].v.output_type == OUTPUT_GRAY)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(output_gray), TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(output_color), TRUE);

#ifdef DO_LINEAR
  if (plist[plist_current].v.linear == 0)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linear_off), TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linear_on), TRUE);
#endif

  switch (plist[plist_current].v.image_type)
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
      plist[plist_current].v.image_type = IMAGE_CONTINUOUS;
      break;
    }

  gtk_preview_update();
}

/****************************************************************************
 *
 * gtk_position_callback() - callback for position entry widgets
 *
 ****************************************************************************/
static void gtk_position_callback(GtkWidget *widget)
{
  int dontcheck = 0;
  if (widget == top_entry)
    {
      gfloat new_value = atof(gtk_entry_get_text(GTK_ENTRY(widget)));
      vars.top = (new_value + 1.0 / 144) * 72;
    }
  else if (widget == left_entry)
    {
      gfloat new_value = atof(gtk_entry_get_text(GTK_ENTRY(widget)));
      vars.left = (new_value + 1.0 / 144) * 72;
    }
  else if (widget == bottom_entry)
    {
      gfloat new_value = atof(gtk_entry_get_text(GTK_ENTRY(widget)));
      if (vars.scaling < 0)
	vars.top =
	  (new_value + 1.0 / 144) * 72 - (image_height * -72.0 / vars.scaling);
      else
	vars.top = (new_value + 1.0 / 144) * 72 - print_height;
    }
  else if (widget == right_entry)
    {
      gfloat new_value = atof(gtk_entry_get_text(GTK_ENTRY(widget)));
      if (vars.scaling < 0)
	vars.left =
	  (new_value + 1.0 / 144) * 72 - (image_width * -72.0 / vars.scaling);
      else
	vars.left = (new_value + 1.0 / 144) * 72 - print_width;
    }
  else if (widget == recenter_button)
    {
      vars.left = -1;
      vars.top = -1;
      dontcheck = 1;
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
  gtk_preview_update();
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
  plist_t	*p;


  plist_current = data;
  p             = plist + plist_current;

  if (p->v.driver[0] != '\0')
  {
    strcpy(vars.driver, p->v.driver);

    current_printer = get_printer_by_driver(vars.driver);
  }

  strcpy(vars.ppd_file, p->v.ppd_file);
  strcpy(vars.media_size, p->v.media_size);
  strcpy(vars.media_type, p->v.media_type);
  strcpy(vars.media_source, p->v.media_source);
  strcpy(vars.ink_type, p->v.ink_type);
  strcpy(vars.resolution, p->v.resolution);
  strcpy(vars.output_to, p->v.output_to);
  gtk_do_misc_updates();

 /*
  * Now get option parameters...
  */

  if (num_media_sizes > 0)
  {
    for (i = 0; i < num_media_sizes; i ++)
      free(media_sizes[i]);
    free(media_sizes);
  }

  media_sizes = (*(current_printer->parameters))(current_printer->model,
						 p->v.ppd_file,
						 "PageSize", &num_media_sizes);
  if (vars.media_size[0] == '\0')
    strcpy(vars.media_size, media_sizes[0]);
  gtk_plist_build_menu(media_size,
		       &media_size_menu,
		       num_media_sizes,
		       media_sizes,
		       p->v.media_size,
		       gtk_media_size_callback);

  if (num_media_types > 0)
  {
    for (i = 0; i < num_media_types; i ++)
      free(media_types[i]);
    free(media_types);
  }

  media_types = (*(current_printer->parameters))(current_printer->model,
						 p->v.ppd_file,
						 "MediaType",
						 &num_media_types);
  if (vars.media_type[0] == '\0' && media_types != NULL)
    strcpy(vars.media_type, media_types[0]);
  else if (media_types == NULL)
    vars.media_type[0] = '\0';
  gtk_plist_build_menu(media_type,
		       &media_type_menu,
		       num_media_types,
		       media_types,
		       p->v.media_type,
		       gtk_media_type_callback);

  if (num_media_sources > 0)
  {
    for (i = 0; i < num_media_sources; i ++)
      free(media_sources[i]);
    free(media_sources);
  }

  media_sources = (*(current_printer->parameters))(current_printer->model,
						   p->v.ppd_file,
						   "InputSlot",
						   &num_media_sources);
  if (vars.media_source[0] == '\0' && media_sources != NULL)
    strcpy(vars.media_source, media_sources[0]);
  else if (media_sources == NULL)
    vars.media_source[0] = '\0';
  gtk_plist_build_menu(media_source,
		       &media_source_menu,
		       num_media_sources,
		       media_sources,
		       p->v.media_source,
		       gtk_media_source_callback);


  if (num_ink_types > 0)
  {
    for (i = 0; i < num_ink_types; i ++)
      free(ink_types[i]);
    free(ink_types);
  }

  ink_types = (*(current_printer->parameters))(current_printer->model,
					       p->v.ppd_file,
					       "InkType", &num_ink_types);
  if (vars.ink_type[0] == '\0' && ink_types != NULL)
    strcpy(vars.ink_type, ink_types[0]);
  else if (ink_types == NULL)
    vars.ink_type[0] = '\0';
  gtk_plist_build_menu(ink_type,
		       &ink_type_menu,
		       num_ink_types,
		       ink_types,
		       p->v.ink_type,
		       gtk_ink_type_callback);

  if (num_resolutions > 0)
  {
    for (i = 0; i < num_resolutions; i ++)
      free(resolutions[i]);
    free(resolutions);
  }

  resolutions = (*(current_printer->parameters))(current_printer->model,
						 p->v.ppd_file,
						 "Resolution",
						 &num_resolutions);
  if (vars.resolution[0] == '\0' && resolutions != NULL)
    strcpy(vars.resolution, resolutions[0]);
  else if (resolutions == NULL)
    vars.resolution[0] = '\0';
  gtk_plist_build_menu(resolution,
		       &resolution_menu,
		       num_resolutions,
		       resolutions,
		       p->v.resolution,
		       gtk_resolution_callback);
}

/****************************************************************************
 *
 * gtk_media_size_callback() - Update the current media size...
 *
 ****************************************************************************/
static void gtk_media_size_callback(GtkWidget *widget, /* I -Media size menu */
				    gint      data)    /* I - Data */
{
  strcpy(vars.media_size, media_sizes[data]);
  strcpy(plist[plist_current].v.media_size, media_sizes[data]);
  vars.left       = -1;
  vars.top        = -1;
  plist[plist_current].v.left = vars.left;
  plist[plist_current].v.top = vars.top;

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
  strcpy(vars.media_type, media_types[data]);
  strcpy(plist[plist_current].v.media_type, media_types[data]);
}


/****************************************************************************
 *
 * gtk_media_source_callback() - Update the current media source...
 *
 ****************************************************************************/
static void gtk_media_source_callback(GtkWidget *widget, /* I-Media source */
				      gint      data)    /* I - Data */
{
  strcpy(vars.media_source, media_sources[data]);
  strcpy(plist[plist_current].v.media_source, media_sources[data]);
}

/****************************************************************************
 *
 * gtk_ink_type_callback() - Update the current media source...
 *
 ****************************************************************************/
static void gtk_ink_type_callback(GtkWidget *widget, /* I-Ink type menu */
				  gint      data)    /* I - Data */
{
  strcpy(vars.ink_type, ink_types[data]);
  strcpy(plist[plist_current].v.ink_type, ink_types[data]);
}

/****************************************************************************
 *
 * gtk_resolution_callback() - Update the current resolution...
 *
 ****************************************************************************/
static void gtk_resolution_callback(GtkWidget *widget, /* I-Media size menu */
				    gint      data)    /* I - Data */
{
  strcpy(vars.resolution, resolutions[data]);
  strcpy(plist[plist_current].v.resolution, resolutions[data]);
}


/****************************************************************************
 *
 * gtk_orientation_callback() - Update the current media size...
 *
 ****************************************************************************/
static void gtk_orientation_callback(GtkWidget *widget,
				     gint      data)
{
  vars.orientation = data;
  vars.left        = -1;
  vars.top         = -1;
  plist[plist_current].v.orientation = vars.orientation;
  plist[plist_current].v.left = vars.left;
  plist[plist_current].v.top = vars.top;

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
    vars.output_type = data;
    plist[plist_current].v.output_type = data;
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
    vars.linear = data;
    plist[plist_current].v.linear = data;
  }
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
    vars.image_type = data;
    plist[plist_current].v.image_type = data;
  }
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

    gtk_widget_destroy(print_dialog);
  }
  else
    gtk_widget_show(file_browser);
}

/****************************************************************************
 *
 * gtk_printandsave_callback() - 
 *
 ****************************************************************************/
static void gtk_printandsave_callback(void)
{
  runme = TRUE;
  saveme = TRUE;
  if (plist_current > 0)
  {
    gtk_widget_destroy(print_dialog);
  }
  else
    gtk_widget_show(file_browser);
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
  int idx;

  current_printer = get_printer_by_driver(plist[plist_current].v.driver);
  idx = get_printer_index_by_driver(plist[plist_current].v.driver);

  gtk_option_menu_set_history(GTK_OPTION_MENU(printer_driver), idx);

  gtk_entry_set_text(GTK_ENTRY(ppd_file), plist[plist_current].v.ppd_file);

  if (strncmp(plist[plist_current].v.driver, "ps", 2) == 0)
    {
      gtk_widget_show(ppd_file);
      gtk_widget_show(ppd_button);
    }
  else
    {
      gtk_widget_hide(ppd_file);
      gtk_widget_hide(ppd_button);
    }

  gtk_entry_set_text(GTK_ENTRY(output_cmd), plist[plist_current].v.output_to);

  if (plist_current == 0)
    gtk_widget_hide(output_cmd);
  else
    gtk_widget_show(output_cmd);

  gtk_widget_show(setup_dialog);
}

/****************************************************************************
 *
 * gtk_setup_ok__callback() - 
 *
 ****************************************************************************/
static void gtk_setup_ok_callback(void)
{
  strcpy(vars.driver, current_printer->driver);
  strcpy(plist[plist_current].v.driver, current_printer->driver);

  strcpy(vars.output_to, gtk_entry_get_text(GTK_ENTRY(output_cmd)));
  strcpy(plist[plist_current].v.output_to, vars.output_to);

  strcpy(vars.ppd_file, gtk_entry_get_text(GTK_ENTRY(ppd_file)));
  strcpy(plist[plist_current].v.ppd_file, vars.ppd_file);

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
static void gtk_print_driver_callback(GtkWidget *widget, /* I - Driver menu */
				      gint      data)    /* I - Data */
{
  current_printer = get_printer_by_index((int) data);

  if (strncmp(current_printer->driver, "ps", 2) == 0)
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
  strcpy(vars.output_to,
         gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_browser)));

  runme = TRUE;

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

  gtk_widget_destroy(print_dialog);
}

/****************************************************************************
 *
 * gtk_preview_update_callback() - 
 *
 ****************************************************************************/
static void gtk_preview_update(void)
{
  int		temp,		/* Swapping variable */
		orient,		/* True orientation of page */
		tw0, tw1,	/* Temporary page_widths */
		th0, th1,	/* Temporary page_heights */
		ta0 = 0, ta1 = 0;	/* Temporary areas */
  int		left, right,	/* Imageable area */
		top, bottom,
		width, length;	/* Physical width */
  static GdkGC	*gc = NULL;	/* Graphics context */
  char s[255];


  if (preview->widget.window == NULL)
    return;

  gdk_window_clear(preview->widget.window);

  if (gc == NULL)
    gc = gdk_gc_new(preview->widget.window);

  (*current_printer->imageable_area)(current_printer->model, vars.ppd_file,
				     vars.media_size, &left, &right,
				     &bottom, &top);

  page_width  = right - left;
  page_height = top - bottom;

  (*current_printer->media_size)(current_printer->model, vars.ppd_file,
				 vars.media_size, &width, &length);

  if (vars.scaling < 0)
  {
    tw0 = 72 * -image_width / vars.scaling;
    th0 = tw0 * image_height / image_width;
    tw1 = tw0;
    th1 = th0;
  }
  else
  {
    /* Portrait */
    tw0 = page_width * vars.scaling / 100;
    th0 = tw0 * image_height / image_width;
    if (th0 > page_height * vars.scaling / 100)
    {
      th0 = page_height * vars.scaling / 100;
      tw0 = th0 * image_width / image_height;
    }
    ta0 = tw0 * th0;

    /* Landscape */
    tw1 = page_height * vars.scaling / 100;
    th1 = tw1 * image_height / image_width;
    if (th1 > page_width * vars.scaling / 100)
    {
      th1 = page_width * vars.scaling / 100;
      tw1 = th1 * image_width / image_height;
    }
    ta1 = tw1 * th1;
  }

  if (vars.orientation == ORIENT_AUTO)
  {
    if (vars.scaling < 0)
    {
      if ((page_width > page_height && tw0 > th0) ||
	  (page_height > page_width && th0 > tw0))
	{
	  orient = ORIENT_PORTRAIT;
	  if (tw0 > page_width)
	    {
	      vars.scaling *= (double) page_width / (double) tw0;
	      th0 = th0 * page_width / tw0;
	    }
	  if (th0 > page_height)
	    {
	      vars.scaling *= (double) page_height / (double) th0;
	      tw0 = tw0 * page_height / th0;
	    }
	}
      else
	{
	  orient = ORIENT_LANDSCAPE;
	  if (tw1 > page_height)
	    {
	      vars.scaling *= (double) page_height / (double) tw1;
	      th1 = th1 * page_height / tw1;
	    }
	  if (th1 > page_width)
	    {
	      vars.scaling *= (double) page_width / (double) th1;
	      tw1 = tw1 * page_width / th1;
	    }
	}
    }
    else
    {
      if (ta0 >= ta1)
	orient = ORIENT_PORTRAIT;
      else
	orient = ORIENT_LANDSCAPE;
    }
  }
  else
    orient = vars.orientation;

  if (orient == ORIENT_LANDSCAPE)
  {
    temp         = page_width;
    page_width   = page_height;
    page_height  = temp;
    temp         = width;
    width        = length;
    length       = temp;
    print_width  = tw1;
    print_height = th1;
  }
  else
  {
    print_width  = tw0;
    print_height = th0;
  }

  page_left = (PREVIEW_SIZE_HORIZ - 10 * page_width / 72) / 2;
  page_top  = (PREVIEW_SIZE_VERT - 10 * page_height / 72) / 2;

  gdk_draw_rectangle(preview->widget.window, gc, 0,
                     (PREVIEW_SIZE_HORIZ - (10 * width / 72)) / 2,
                     (PREVIEW_SIZE_VERT - (10 * length / 72)) / 2,
                     10 * width / 72, 10 * length / 72);


  if (vars.left < 0)
    vars.left = (page_width - print_width) / 2;

  left = vars.left;

  if (left > (page_width - print_width))
    {
      left      = page_width - print_width;
      vars.left = left;
      plist[plist_current].v.left = vars.left;
    }

  if (vars.top < 0)
    vars.top  = (page_height - print_height) / 2;
  top  = vars.top;

  if (top > (page_height - print_height))
    {
      top      = page_height - print_height;
      vars.top = top;
      plist[plist_current].v.top = vars.top;
    }
  
  sprintf(s, "%.2f", vars.top / 72.0);
  gtk_signal_handler_block_by_data(GTK_OBJECT(top_entry), NULL);
  gtk_entry_set_text(GTK_ENTRY(top_entry), s);
  gtk_signal_handler_unblock_by_data(GTK_OBJECT(top_entry), NULL);

  sprintf(s, "%.2f", vars.left / 72.0);
  gtk_signal_handler_block_by_data(GTK_OBJECT(left_entry), NULL);
  gtk_entry_set_text(GTK_ENTRY(left_entry), s);
  gtk_signal_handler_unblock_by_data(GTK_OBJECT(left_entry), NULL);

  gtk_signal_handler_block_by_data(GTK_OBJECT(bottom_entry), NULL);
  if (vars.scaling < 0)
    sprintf(s, "%.2f",
	    (vars.top + (image_height * -72.0 / vars.scaling)) / 72.0);
  else
    sprintf(s, "%.2f", (vars.top + print_height) / 72.0);
  gtk_entry_set_text(GTK_ENTRY(bottom_entry), s);
  gtk_signal_handler_unblock_by_data(GTK_OBJECT(bottom_entry), NULL);

  gtk_signal_handler_block_by_data(GTK_OBJECT(right_entry), NULL);
  if (vars.scaling < 0)
    sprintf(s, "%.2f",
	    (vars.left + (image_width * -72.0 / vars.scaling)) / 72.0);
  else
    sprintf(s, "%.2f", (vars.left + print_width) / 72.0);
  gtk_entry_set_text(GTK_ENTRY(right_entry), s);
  gtk_signal_handler_unblock_by_data(GTK_OBJECT(right_entry), NULL);

  gdk_draw_rectangle(preview->widget.window, gc, 1,
		     page_left + 10 * left / 72, page_top + 10 * top / 72,
                     10 * print_width / 72, 10 * print_height / 72);

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
}

/****************************************************************************
 *
 * gtk_preview_motion_callback() - 
 *
 ****************************************************************************/
static void gtk_preview_motion_callback(GtkWidget      *w,
					GdkEventMotion *event)
{
  if (vars.left < 0 || vars.top < 0)
  {
    vars.left = 72 * (page_width - print_width) / 20;
    vars.top  = 72 * (page_height - print_height) / 20;
  }

  vars.left += 72 * (event->x - mouse_x) / 10;
  vars.top  += 72 * (event->y - mouse_y) / 10;

  if (vars.left < 0)
    vars.left = 0;

  if (vars.top < 0)
    vars.top = 0;
  plist[plist_current].v.left = vars.left;
  plist[plist_current].v.top = vars.top;

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

#endif
