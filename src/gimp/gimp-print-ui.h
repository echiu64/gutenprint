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

#ifndef __GIMP_PRINT_UI_H__
#define __GIMP_PRINT_UI_H__

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

#define ORIENT_AUTO             -1      /* Best orientation */
#define ORIENT_PORTRAIT         0       /* Portrait orientation */
#define ORIENT_LANDSCAPE        1       /* Landscape orientation */
#define ORIENT_UPSIDEDOWN       2       /* Reverse portrait orientation */
#define ORIENT_SEASCAPE         3       /* Reverse landscape orientation */

typedef struct		/**** Printer List ****/
{
  int	active;			/* Do we know about this printer? */
  char	*name;			/* Name of printer */
  char  *output_to;
  float	scaling;		/* Scaling, percent of printable area */
  int   orientation;
  int	unit;			/* Units for preview area 0=Inch 1=Metric */
  int	invalid_mask;
  stp_vars_t v;
} gp_plist_t;

extern gint             plist_count;	   /* Number of system printers */
extern gint             plist_current;     /* Current system printer */
extern gp_plist_t         *plist;		  /* System printers */
extern stp_printer_t current_printer;
extern gint             runme;
extern gint             saveme;

extern gp_plist_t *pv;
extern gp_plist_t gimp_vars;

/*
 * Function prototypes
 */
extern void plist_set_output_to(gp_plist_t *p, const char *val);
extern void plist_set_output_to_n(gp_plist_t *p, const char *val, int n);
extern const char *plist_get_output_to(const gp_plist_t *p);
extern void plist_set_name(gp_plist_t *p, const char *val);
extern void plist_set_name_n(gp_plist_t *p, const char *val, int n);
extern const char *plist_get_name(const gp_plist_t *p);
extern void copy_printer(gp_plist_t *vd, const gp_plist_t *vs);
extern int add_printer(const gp_plist_t *key, int add_only);
extern void initialize_printer(gp_plist_t *printer);

extern void set_printrc_file(const char *name);
extern const char * get_printrc_file(void);
extern void printrc_load (void);
extern void get_system_printers (void);
extern void printrc_save (void);
extern void set_image_filename(const char *);
extern const char *get_image_filename(void);
extern void set_errfunc(stp_outfunc_t wfunc);
extern stp_outfunc_t get_errfunc(void);
extern void set_errdata(void *errdata);
extern void *get_errdata(void);

extern void create_main_window (void);

extern gint compute_orientation(void);
extern void set_image_dimensions(gint width, gint height);
extern void set_image_resolution(gdouble xres, gdouble yres);

typedef guchar *(*get_thumbnail_func_t)(void *data, gint *width, gint *height,
					gint *bpp, gint page);
extern void set_thumbnail_func(get_thumbnail_func_t);
extern get_thumbnail_func_t get_thumbnail_func(void);
extern void set_thumbnail_data(void *);
extern void *get_thumbnail_data(void);

#endif  /* __GIMP_PRINT_UI_H__ */
