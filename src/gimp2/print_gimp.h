/*
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
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *
 * Revision History:
 *
 *   See ChangeLog
 */

#ifndef __PRINT_GIMP_H__
#define __PRINT_GIMP_H__

#ifdef __GNUC__
#ifndef inline
#define inline __inline__
#endif
#endif

#include <gutenprint/gutenprint.h>
#include <gutenprintui2/gutenprintui.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#pragma GCC diagnostic pop

/* How to create an Image wrapping a Gimp drawable */
extern stpui_image_t *Image_GimpDrawable_new(GimpDrawable *drawable, gint32);

extern void do_gimp_install_procedure(const char *blurb, const char *help,
				      const char *auth, const char *copy,
				      const char *types, int n_args,
				      GimpParamDef *args);

/*
 * Work around GIMP library not being const-safe.  This is a very ugly
 * hack, but the excessive warnings generated can mask more serious
 * problems.
 */

#define BAD_CONST_CHAR char *

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
static inline gint
p2gint(void *p)
{
  return (gint) p;
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
static inline void *
gint2p(int i)
{
  return (gpointer) i;
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic ignored "-Woverlength-strings"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
static inline void *
cast_safe(const void *ptr)
{
  return (void *)ptr;
}
#pragma GCC diagnostic pop

#endif  /* __PRINT_GIMP_H__ */
