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

#include "print_gimp.h"

#include "print-intl.h"
#include <string.h>

gint    thumbnail_w, thumbnail_h, thumbnail_bpp;
guchar *thumbnail_data;
gint    adjusted_thumbnail_bpp;
guchar *adjusted_thumbnail_data;
guchar *preview_thumbnail_data;

GtkWidget *color_adjust_dialog;

GtkWidget   *dither_algo_combo       = NULL;
GtkWidget   *dither_algo_label       = NULL;
static gint  dither_algo_callback_id = -1;
static void color_update (GtkAdjustment *adjustment);

typedef struct
{
  const char *name;
  GtkObject *adjustment;
  gfloat scale;
  gint is_color;
  gint update_thumbnail;
} color_option_t;

static color_option_t color_options[] =
  {
    { "Brightness", NULL, 10,  0, 1 },
    { "Contrast",   NULL, 10,  0, 1 },
    { "Cyan",       NULL, 10,  1, 1 },
    { "Magenta",    NULL, 10,  1, 1 },
    { "Yellow",     NULL, 10,  1, 1 },
    { "Saturation", NULL, 100, 1, 1 },
    { "Density",    NULL, 100, 0, 0 },
    { "Gamma",      NULL, 100, 0, 1 }
  };
const static gint color_option_count = (sizeof(color_options) /
					sizeof(color_option_t));

static void set_color_defaults (void);

static void dither_algo_callback (GtkWidget *widget, gpointer data);

void build_dither_combo               (void);

static GtkDrawingArea *swatch = NULL;

#define SWATCH_W (128)
#define SWATCH_H (128)

static void
dither_algo_callback (GtkWidget *widget, gpointer data)
{
  stp_string_list_t vec = NULL;
  stp_parameter_t desc;
  const gchar *new_algo;
  stp_describe_parameter(pv->v, "DitherAlgorithm", &desc);
  new_algo = Combo_get_name(dither_algo_combo, desc.bounds.str);
  if (strcmp(stp_get_string_parameter(pv->v, "DitherAlgorithm"), new_algo) != 0)
    stp_set_string_parameter(pv->v, "DitherAlgorithm", new_algo);
}

void
build_dither_combo (void)
{
  stp_string_list_t vec = NULL;
  stp_parameter_t desc;
  stp_describe_parameter(pv->v, "DitherAlgorithm", &desc);
  if (desc.type == STP_PARAMETER_TYPE_STRING_LIST)
    {
      vec = desc.bounds.str;
      if (vec == NULL || stp_string_list_count(vec) == 0)
	stp_set_string_parameter(pv->v, "DitherAlgorithm", NULL);
      else if (stp_get_string_parameter(pv->v, "DitherAlgorithm")[0] == '\0')
	stp_set_string_parameter(pv->v, "DitherAlgorithm", desc.deflt.str);
    }

  plist_build_combo (dither_algo_combo,
		     dither_algo_label,
		     vec,
		     stp_get_string_parameter (pv->v, "DitherAlgorithm"),
		     desc.deflt.str,
		     &dither_algo_callback,
		     &dither_algo_callback_id,
		     NULL);
  if (vec)
    stp_string_list_free(vec);
}

void
redraw_color_swatch (void)
{
  static GdkGC *gc = NULL;
  static GdkColormap *cmap;

  if (swatch == NULL || swatch->widget.window == NULL)
    return;

  if (gc == NULL)
    {
      gc = gdk_gc_new (swatch->widget.window);
      cmap = gtk_widget_get_colormap (GTK_WIDGET(swatch));
    }

  (adjusted_thumbnail_bpp == 1
   ? gdk_draw_gray_image
   : gdk_draw_rgb_image) (swatch->widget.window, gc,
			  (SWATCH_W - thumbnail_w) / 2,
			  (SWATCH_H - thumbnail_h) / 2,
			  thumbnail_w, thumbnail_h, GDK_RGB_DITHER_NORMAL,
			  adjusted_thumbnail_data,
			  adjusted_thumbnail_bpp * thumbnail_w);
}

/*
 * create_color_adjust_window (void)
 *
 * NOTES:
 *   creates the color adjuster popup, allowing the user to adjust brightness,
 *   contrast, saturation, etc.
 */
void
create_color_adjust_window (void)
{
  gint i;
  GtkWidget *table;
  GtkWidget *event_box;
#if 0
  GtkWidget *curve;
#endif

  /*
   * Fetch a thumbnail of the image we're to print from the Gimp.  This must
   *
   */

  thumbnail_w = THUMBNAIL_MAXW;
  thumbnail_h = THUMBNAIL_MAXH;
  thumbnail_data =
    get_thumbnail_data (&thumbnail_w, &thumbnail_h, &thumbnail_bpp);

  /*
   * thumbnail_w and thumbnail_h have now been adjusted to the actual
   * thumbnail dimensions.  Now initialize a color-adjusted version of
   * the thumbnail.
   */

  adjusted_thumbnail_data = g_malloc (3 * thumbnail_w * thumbnail_h);
  preview_thumbnail_data = g_malloc (3 * thumbnail_w * thumbnail_h);

  color_adjust_dialog =
    gimp_dialog_new (_("Print Color Adjust"), "print",
		     gimp_standard_help_func, "filters/print.html",
		     GTK_WIN_POS_MOUSE, FALSE, TRUE, FALSE,

		     _("Set Defaults"), set_color_defaults,
		     NULL, NULL, NULL, FALSE, FALSE,
		     _("Close"), gtk_widget_hide,
		     NULL, 1, NULL, TRUE, TRUE,

		     NULL);
  gtk_window_set_policy(GTK_WINDOW(color_adjust_dialog), 1, 1, 1);

  table = gtk_table_new (1, 1, FALSE);
  gtk_container_set_resize_mode(GTK_CONTAINER(table), GTK_RESIZE_IMMEDIATE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 0);
/*  gtk_table_set_row_spacing (GTK_TABLE (table), 8, 6); */

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (color_adjust_dialog)->vbox),
		      table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*
   * Drawing area for color swatch feedback display...
   */

  event_box = gtk_event_box_new ();
  gtk_widget_show (GTK_WIDGET (event_box));
  gtk_table_attach (GTK_TABLE (table), GTK_WIDGET (event_box),
                    0, 3, 0, 1, 0, 0, 0, 0);

  swatch = (GtkDrawingArea *) gtk_drawing_area_new ();
  gtk_widget_set_events (GTK_WIDGET (swatch), GDK_EXPOSURE_MASK);
  gtk_drawing_area_size (swatch, SWATCH_W, SWATCH_H);
  gtk_container_add (GTK_CONTAINER (event_box), GTK_WIDGET (swatch));
  gtk_widget_show (GTK_WIDGET (swatch));

  set_help_data (GTK_WIDGET (event_box), _("Image preview"));
  gtk_signal_connect (GTK_OBJECT (swatch), "expose_event",
                      GTK_SIGNAL_FUNC (redraw_color_swatch),
                      NULL);

  for (i = 0; i < color_option_count; i++)
    {
      color_option_t *opt = &(color_options[i]);
      stp_parameter_t desc;
      stp_describe_parameter(stp_default_settings(), opt->name, &desc);
      if (desc.type == STP_PARAMETER_TYPE_DOUBLE &&
	  desc.class == STP_PARAMETER_CLASS_OUTPUT &&
	  desc.level == STP_PARAMETER_LEVEL_BASIC)
	{
	  opt->adjustment =
	    gimp_scale_entry_new(GTK_TABLE(table), 0, i + 1, _(desc.text),
				 200, 0, desc.deflt.dbl,
				 desc.bounds.dbl.lower,
				 desc.bounds.dbl.upper,
				 desc.deflt.dbl / (opt->scale * 10),
				 desc.deflt.dbl / opt->scale,
				 3, TRUE, 0, 0, NULL, NULL);
	  set_adjustment_tooltip(opt->adjustment, _(desc.help));
	  gtk_signal_connect(GTK_OBJECT(opt->adjustment), "value_changed",
			     GTK_SIGNAL_FUNC(color_update), (gpointer) i);
	}
    }

  /*
   * Dither algorithm option combo...
   */

  event_box = gtk_event_box_new ();
  dither_algo_label = table_attach_aligned
    (GTK_TABLE (table), 0, color_option_count + 1,
     _("Dither Algorithm:"), 1.0, 0.5, event_box, 1, TRUE);

  dither_algo_combo = gtk_combo_new ();
  gtk_container_add (GTK_CONTAINER(event_box), dither_algo_combo);
  gtk_widget_show (dither_algo_combo);

  set_help_data(GTK_WIDGET (event_box),
		_("Choose the dither algorithm to be used.\n"
		  "Adaptive Hybrid usually produces the best "
		  "all-around quality.\n"
		  "EvenTone is a new, experimental algorithm "
		  "that often produces excellent results.\n"
		  "Ordered is faster and produces almost as good "
		  "quality on photographs.\n"
		  "Fast and Very Fast are considerably faster, and "
		  "work well for text and line art.\n"
		  "Hybrid Floyd-Steinberg generally produces "
		  "inferior output."));
#if 0
  curve = gtk_gamma_curve_new();
  table_attach_aligned(GTK_TABLE (table), 0, color_option_count + 2,
		       _("Curve:"), 1.0, 0.5, curve, 1, TRUE);
  gtk_curve_set_range(GTK_CURVE(GTK_GAMMA_CURVE(curve)->curve), 0.0, 200.0, 0.0, 200.0);
  gtk_widget_show(curve);
#endif
}

static void
color_update (GtkAdjustment *adjustment)
{
  int i;
  for (i = 0; i < color_option_count; i++)
    {
      color_option_t *opt = &(color_options[i]);
      if (adjustment == GTK_ADJUSTMENT(opt->adjustment))
	{
	  if (opt->update_thumbnail)
	    invalidate_preview_thumbnail ();
	  if (stp_get_float_parameter(pv->v, opt->name) != adjustment->value)
	    {
	      stp_set_float_parameter(pv->v, opt->name, adjustment->value);
	      if (opt->update_thumbnail)
		update_adjusted_thumbnail();
	    }
	}
    }
}

void
set_color_sliders_active (gboolean active)
{
  int i;
  for (i = 0; i < color_option_count; i++)
    {
      if (color_options[i].is_color)
	{
	  GtkObject *adj = color_options[i].adjustment;
	  gtk_widget_set_sensitive
	    (GTK_WIDGET (GIMP_SCALE_ENTRY_LABEL (adj)), active);
	  gtk_widget_set_sensitive
	    (GTK_WIDGET (GIMP_SCALE_ENTRY_SCALE (adj)), active);
	  gtk_widget_set_sensitive
	    (GTK_WIDGET (GIMP_SCALE_ENTRY_SPINBUTTON (adj)), active);
	}
    }
}

void
do_color_updates (void)
{
  int i;
  for (i = 0; i < color_option_count; i++)
    {
      color_option_t *opt = &(color_options[i]);
      gtk_adjustment_set_value(GTK_ADJUSTMENT(opt->adjustment),
			       stp_get_float_parameter(pv->v, opt->name));
    }
  update_adjusted_thumbnail ();
}

void
set_color_defaults (void)
{
  const stp_vars_t defvars = stp_default_settings ();
  int i;
  for (i = 0; i < color_option_count; i++)
    {
      color_option_t *opt = &(color_options[i]);
      stp_set_float_parameter(pv->v, opt->name,
			      stp_get_float_parameter(defvars, opt->name));
    }

  do_color_updates ();
}
