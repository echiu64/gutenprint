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
GtkWidget* print_dialog;           /* Print dialog window */
GtkWidget* recenter_button;
GtkWidget* left_entry;
GtkWidget* right_entry;
GtkWidget* top_entry;
GtkWidget* bottom_entry;


GtkDrawingArea	*preview;		/* Preview drawing area widget */
int		mouse_x,		/* Last mouse X */
		mouse_y;		/* Last mouse Y */

int 		page_left;		/* Left pixel column of page */
int		page_top;		/* Top pixel row of page */
int		page_width;		/* Width of page on screen */
int		page_height;		/* Height of page on screen */
int		print_width;		/* Printed width of image */
int		print_height;		/* Printed height of image */

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
static void gtk_linear_callback(GtkWidget *, gint);
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

static void gtk_show_adjust_button_callback(GtkWidget *);

extern void gtk_create_color_adjust_window(void);

GtkWidget* table;      /* Table "container" for controls */
GtkWidget* dialog;     /* Dialog window */
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
  gtk_signal_connect(GTK_OBJECT((GtkWidget *)preview), "expose_event",
		     (GtkSignalFunc)gtk_preview_update,
		     NULL);
  gtk_signal_connect(GTK_OBJECT((GtkWidget *)preview), "button_press_event",
		     (GtkSignalFunc)gtk_preview_button_callback,
		     NULL);
  gtk_signal_connect(GTK_OBJECT((GtkWidget *)preview), "button_release_event",
		     (GtkSignalFunc)gtk_preview_button_callback,
		     NULL);
  gtk_signal_connect(GTK_OBJECT((GtkWidget *)preview), "motion_notify_event",
		     (GtkSignalFunc)gtk_preview_motion_callback,
		     NULL);
  gtk_widget_show((GtkWidget *)preview);

  gtk_widget_set_events((GtkWidget *)preview,
                        GDK_EXPOSURE_MASK | GDK_BUTTON_MOTION_MASK |
                        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  recenter_button = button = gtk_button_new_with_label(_("Center Image"));
  gtk_table_attach(GTK_TABLE(table),
		   button, 1, 2, 7, 8, GTK_FILL, GTK_FILL, 0, 0);
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

}

/****************************************************************************
 *
 * GTK_scaling_callback() - Update the scaling scale using the text entry.
 *
 ****************************************************************************/
static void gtk_scaling_callback(GtkWidget *widget)	/* I - Entry widget */
{
}


/****************************************************************************
 *
 * gtk_scaling_update() - Update the scaling field using the scale.
 *
 ****************************************************************************/
static void gtk_scaling_update(GtkAdjustment *adjustment) /* I - New value */
{
}

/****************************************************************************
 *
 * gtk_plist_build_menu() - Build an option menu for the given parameters...
 *
 ****************************************************************************/
static void gtk_do_misc_updates()
{
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
}

/****************************************************************************
 *
 * gtk_plist_callback() - Update the current system printer...
 *
 ****************************************************************************/
static void gtk_plist_callback(GtkWidget *widget, /* I - Driver option menu */
			       gint      data)    /* I - Data */
{
}

/****************************************************************************
 *
 * gtk_media_size_callback() - Update the current media size...
 *
 ****************************************************************************/
static void gtk_media_size_callback(GtkWidget *widget, /* I -Media size menu */
				    gint      data)    /* I - Data */
{
}


/****************************************************************************
 *
 * gtk_media_type_callback() - Update the current media type...
 *
 ****************************************************************************/
static void gtk_media_type_callback(GtkWidget *widget, /* I- Media type menu */
				    gint      data)    /* I - Data */
{
}


/****************************************************************************
 *
 * gtk_media_source_callback() - Update the current media source...
 *
 ****************************************************************************/
static void gtk_media_source_callback(GtkWidget *widget, /* I-Media source */
				      gint      data)    /* I - Data */
{
}

/****************************************************************************
 *
 * gtk_ink_type_callback() - Update the current media source...
 *
 ****************************************************************************/
static void gtk_ink_type_callback(GtkWidget *widget, /* I-Ink type menu */
				  gint      data)    /* I - Data */
{
}

/****************************************************************************
 *
 * gtk_resolution_callback() - Update the current resolution...
 *
 ****************************************************************************/
static void gtk_resolution_callback(GtkWidget *widget, /* I-Media size menu */
				    gint      data)    /* I - Data */
{
}


/****************************************************************************
 *
 * gtk_orientation_callback() - Update the current media size...
 *
 ****************************************************************************/
static void gtk_orientation_callback(GtkWidget *widget,
				     gint      data)
{
}


/****************************************************************************
 *
 * gtk_output_type_callback() - Update the current output type...
 *
 ****************************************************************************/
static void gtk_output_type_callback(GtkWidget *widget,
				     gint      data)
{
}

/****************************************************************************
 *
 * gtk_linear_callback() - Update the current linear gradient mode...
 *
 ****************************************************************************/
static void gtk_linear_callback(GtkWidget *widget, /* I - Output type button */
				gint      data)    /* I - Data */
{
}

/****************************************************************************
 *
 * gtk_print_callback() - Start the print...
 *
 ****************************************************************************/
static void gtk_print_callback(void)
{
}

/****************************************************************************
 *
 * gtk_printandsave_callback() - 
 *
 ****************************************************************************/
static void gtk_printandsave_callback(void)
{
}

/****************************************************************************
 *
 * gtk_save_callback() - save settings, don't destroy dialog
 *
 ****************************************************************************/
static void gtk_save_callback(void)
{
}

/****************************************************************************
 *
 * gtk_cancel_callback() - Cancel the print...
 *
 ****************************************************************************/
static void gtk_cancel_callback(void)
{
}

/****************************************************************************
 *
 * gtk_close_callback() - Exit the print dialog application.
 *
 ****************************************************************************/
static void gtk_close_callback(void)
{
}

/****************************************************************************
 *
 * gtk_setup_open__callback() - 
 *
 ****************************************************************************/
static void gtk_setup_open_callback(void)
{
}

/****************************************************************************
 *
 * gtk_setup_ok__callback() - 
 *
 ****************************************************************************/
static void gtk_setup_ok_callback(void)
{
}

/****************************************************************************
 *
 * gtk_setup_cancel_callback() - 
 *
 ****************************************************************************/
static void gtk_setup_cancel_callback(void)
{
}

/****************************************************************************
 *
 * print_driver_callback() - Update the current printer driver...
 *
 ****************************************************************************/
static void gtk_print_driver_callback(GtkWidget *widget, /* I - Driver menu */
				      gint      data)    /* I - Data */
{
}

/****************************************************************************
 *
 * gtk_ppd_browse_callback() - 
 *
 ****************************************************************************/
static void gtk_ppd_browse_callback(void)
{
}

/****************************************************************************
 *
 * gtk_ppd_ok_callback() - 
 *
 ****************************************************************************/
static void gtk_ppd_ok_callback(void)
{
}

/****************************************************************************
 *
 * gtk_ppd_cancel_callback() - 
 *
 ****************************************************************************/
static void gtk_ppd_cancel_callback(void)
{
}

/****************************************************************************
 *
 * gtk_file_ok_callback() - print to file and go away
 *
 ****************************************************************************/
static void gtk_file_ok_callback(void)
{
}

/****************************************************************************
 *
 * gtk_file_cancel_callback() - 
 *
 ****************************************************************************/
static void gtk_file_cancel_callback(void)
{
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
