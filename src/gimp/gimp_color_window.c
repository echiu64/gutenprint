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
static gint  dither_algo_callback_id = -1;
static void color_update (GtkAdjustment *adjustment);

typedef struct
{
  const char *name;
  const char *help;
  gfloat (*accessor)(const stp_vars_t);
  void (*mutator)(stp_vars_t, gfloat);
  GtkObject *adjustment;
  gfloat scale;
  gint is_color;
  gint update_thumbnail;
} color_option_t;

static color_option_t color_options[] =
  {
    { N_("Brightness:"), N_("Set the brightness of the print.\n"
                            "0 is solid black, 2 is solid white"),
      stp_get_brightness, stp_set_brightness, NULL, 10, 0, 1 },
    { N_("Contrast:"), N_("Set the contrast of the print"),
      stp_get_contrast, stp_set_contrast, NULL, 10, 0, 1 },
    { N_("Cyan:"), N_("Set the cyan balance of the print"),
      stp_get_cyan, stp_set_cyan, NULL, 10, 1, 1 },
    { N_("Magenta:"), N_("Set the magenta balance of the print"),
      stp_get_magenta, stp_set_magenta, NULL, 10, 1, 1 },
    { N_("Yellow:"), N_("Set the yellow balance of the print"),
      stp_get_yellow, stp_set_yellow, NULL, 10, 1, 1 },
    { N_("Saturation"), N_("Adjust the saturation (color balance) of the print\n"
			   "Use zero saturation to produce grayscale output "
			   "using color and black inks"),
      stp_get_saturation, stp_set_saturation, NULL, 100, 1, 1 },
    { N_("Density:"), N_("Adjust the density (amount of ink) of the print. "
			 "Reduce the density if the ink bleeds through the "
			 "paper or smears; increase the density if black "
			 "regions are not solid."),
      stp_get_density, stp_set_density, NULL, 100, 0, 0 },
    { N_("Gamma"), N_("Adjust the gamma of the print. Larger values will "
		      "produce a generally brighter print, while smaller "
		      "values will produce a generally darker print. "
		      "Black and white will remain the same, unlike with "
		      "the brightness adjustment."),
      stp_get_gamma, stp_set_gamma, NULL, 100, 0, 1 }
  };

const static gint color_option_count = (sizeof(color_options) /
					sizeof(color_option_t));
static void set_color_defaults (void);

static void dither_algo_callback (GtkWidget *widget, gpointer data);

void build_dither_combo               (void);

static GtkDrawingArea *swatch = NULL;

#define SWATCH_W (128)
#define SWATCH_H (128)

static char *
c_strdup(const char *s)
{
  char *ret = malloc(strlen(s) + 1);
  if (!s)
    exit(1);
  strcpy(ret, s);
  return ret;
}

static void
dither_algo_callback (GtkWidget *widget, gpointer data)
{
  const gchar *new_algo =
    gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (dither_algo_combo)->entry));
  int i;

  for (i = 0; i < stp_dither_algorithm_count (); i ++)
    if (strcasecmp (new_algo, stp_dither_algorithm_text (i)) == 0)
      {
        stp_set_dither_algorithm (pv->v, stp_dither_algorithm_name (i));
        break;
      }
}

void
build_dither_combo (void)
{
  int i;
  stp_param_t *vec = xmalloc(sizeof(stp_param_t) * stp_dither_algorithm_count());

  for (i = 0; i < stp_dither_algorithm_count(); i++)
    {
      vec[i].name = c_strdup (stp_dither_algorithm_name (i));
      vec[i].text = c_strdup (stp_dither_algorithm_text (i));
    }

  plist_build_combo (dither_algo_combo,
		     stp_dither_algorithm_count (),
		     vec,
		     stp_get_dither_algorithm (pv->v),
		     stp_default_dither_algorithm (),
		     &dither_algo_callback,
		     &dither_algo_callback_id,
		     NULL);

  for (i = 0; i < stp_dither_algorithm_count (); i++)
    {
      free ((void *) vec[i].name);
      free ((void *) vec[i].text);
    }
  free (vec);
}

void
redraw_color_swatch (void)
{
  static GdkGC *gc = NULL;
  static GdkColormap *cmap;

  if (swatch == NULL || swatch->widget.window == NULL)
    return;

#if 0
  gdk_window_clear (swatch->widget.window);
#endif

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
  const stp_vars_t lower   = stp_minimum_settings ();
  const stp_vars_t upper   = stp_maximum_settings ();
  const stp_vars_t defvars = stp_default_settings ();

  /*
   * Fetch a thumbnail of the image we're to print from the Gimp.  This must
   *
   */

  thumbnail_w = THUMBNAIL_MAXW;
  thumbnail_h = THUMBNAIL_MAXH;
  thumbnail_data = gimp_image_get_thumbnail_data (image_ID, &thumbnail_w,
                                                  &thumbnail_h, &thumbnail_bpp);

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

  table = gtk_table_new (color_option_count + 2, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 8, 6);
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

  gimp_help_set_help_data (GTK_WIDGET (event_box), _("Image preview"), NULL);
  gtk_signal_connect (GTK_OBJECT (swatch), "expose_event",
                      GTK_SIGNAL_FUNC (redraw_color_swatch),
                      NULL);

  for (i = 0; i < color_option_count; i++)
    {
      color_option_t *opt = &(color_options[i]);
      opt->adjustment =
	gimp_scale_entry_new(GTK_TABLE(table), 0, i + 1, _(opt->name), 200, 0,
			     (opt->accessor)(defvars),
			     (opt->accessor)(lower),
			     (opt->accessor)(upper),
			     (opt->accessor)(defvars) / (opt->scale * 10),
			     (opt->accessor)(defvars) / opt->scale,
			     3, TRUE, 0, 0, NULL, NULL);
      set_adjustment_tooltip(opt->adjustment, _(opt->help), NULL);
      gtk_signal_connect(GTK_OBJECT(opt->adjustment), "value_changed",
			 GTK_SIGNAL_FUNC(color_update), (gpointer) i);
    }

  /*
   * Dither algorithm option combo...
   */

  event_box = gtk_event_box_new ();
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 9,
                             _("Dither Algorithm:"), 1.0, 0.5,
                             event_box, 1, TRUE);

  dither_algo_combo = gtk_combo_new ();
  gtk_container_add (GTK_CONTAINER(event_box), dither_algo_combo);
  gtk_widget_show (dither_algo_combo);

  gimp_help_set_help_data (GTK_WIDGET (event_box),
                           _("Choose the dither algorithm to be used.\n"
                             "Adaptive Hybrid usually produces the best "
                             "all-around quality.\n"
                             "Ordered is faster and produces almost as good "
                             "quality on photographs.\n"
                             "Fast and Very Fast are considerably faster, and "
                             "work well for text and line art.\n"
                             "Hybrid Floyd-Steinberg generally produces "
                             "inferior output."),
                           NULL);

  build_dither_combo ();
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
	  if ((opt->accessor)(pv->v) != adjustment->value)
	    {
	      (opt->mutator)(pv->v, adjustment->value);
	      if (opt->update_thumbnail)
		update_adjusted_thumbnail();
	    }
	}
    }
}

static void
set_adjustment_active (GtkObject *adj,
		       gboolean   active)
{
  gtk_widget_set_sensitive (GTK_WIDGET (GIMP_SCALE_ENTRY_LABEL (adj)), active);
  gtk_widget_set_sensitive (GTK_WIDGET (GIMP_SCALE_ENTRY_SCALE (adj)), active);
  gtk_widget_set_sensitive (GTK_WIDGET (GIMP_SCALE_ENTRY_SPINBUTTON (adj)),
                            active);
}

void
set_color_sliders_active (gboolean active)
{
  int i;
  for (i = 0; i < color_option_count; i++)
    {
      color_option_t *opt = &(color_options[i]);
      if (opt->is_color)
	set_adjustment_active(opt->adjustment, active);
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
			       (opt->accessor)(pv->v));
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
      (opt->mutator)(pv->v, (opt->accessor)(defvars));
    }

  do_color_updates ();
}
