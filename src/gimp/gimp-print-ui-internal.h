/*
 * "$Id$"
 *
 *   Print plug-in for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu). and Steve Miller (smiller@rni.net
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
 *
 * Revision History:
 *
 *   See ChangeLog
 */

#ifndef __GIMP_PRINT_UI_INTERNAL_H__
#define __GIMP_PRINT_UI_INTERNAL_H__

#ifdef __GNUC__
#define inline __inline__
#endif

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#ifdef INCLUDE_GIMP_PRINT_H
#include INCLUDE_GIMP_PRINT_H
#else
#include <gimp-print/gimp-print.h>
#endif

/*
 * All Gimp-specific code is in this file.
 */

#define PLUG_IN_VERSION		VERSION " - " RELEASE_DATE
#define PLUG_IN_NAME		"Print"

#define INVALID_TOP 1
#define INVALID_LEFT 2

extern gint    thumbnail_w, thumbnail_h, thumbnail_bpp;
extern guchar *thumbnail_data;
extern gint    adjusted_thumbnail_bpp;
extern guchar *adjusted_thumbnail_data;
extern guchar *preview_thumbnail_data;

extern GtkWidget *color_adjust_dialog;
extern GtkWidget *dither_algo_combo;

extern stpui_plist_t *pv;

/*
 * Function prototypes
 */
extern void stpui_plist_set_output_to(stpui_plist_t *p, const char *val);
extern void stpui_plist_set_output_to_n(stpui_plist_t *p, const char *val, int n);
extern const char *stpui_plist_get_output_to(const stpui_plist_t *p);
extern void stpui_plist_set_name(stpui_plist_t *p, const char *val);
extern void stpui_plist_set_name_n(stpui_plist_t *p, const char *val, int n);
extern const char *stpui_plist_get_name(const stpui_plist_t *p);
extern void stpui_plist_copy(stpui_plist_t *vd, const stpui_plist_t *vs);

extern int stpui_plist_add(const stpui_plist_t *key, int add_only);
extern void stpui_printer_initialize(stpui_plist_t *printer);
extern void update_adjusted_thumbnail (void);
extern const char *Combo_get_name(GtkWidget   *combo,
				  const stp_string_list_t options);
extern void plist_build_combo         (GtkWidget     *combo,
				       GtkWidget     *label,
				       stp_string_list_t items,
				       const gchar   *cur_item,
				       const gchar	  *def_value,
				       GtkSignalFunc callback,
				       gint          *callback_id,
				       gpointer	     data);

extern void invalidate_frame(void);
extern void invalidate_preview_thumbnail(void);
extern void do_color_updates    (void);
extern void redraw_color_swatch (void);
extern void build_dither_combo  (void);
extern void create_color_adjust_window  (void);
extern void update_adjusted_thumbnail   (void);
extern void set_color_sliders_active(int active);
extern void gimp_writefunc (void *file, const char *buf, size_t bytes);
extern void set_adjustment_tooltip(GtkObject *adjustment, const gchar *tip);
extern void set_help_data(GtkWidget *widget, const gchar *tooltip);
extern GtkWidget *table_attach_aligned(GtkTable *table, gint column, gint row,
				       const gchar *label_text, gfloat xalign,
				       gfloat yalign, GtkWidget *widget,
				       gint colspan, gboolean left_align);

/* Thumbnails -- keep it simple! */

stp_image_t *Image_Thumbnail_new(const guchar *data, gint w, gint h, gint bpp);

#endif  /* __GIMP_PRINT_UI_INTERNAL_H__ */
