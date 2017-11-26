/*
 *
 *   Print plug-in for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gutenprintui2/gutenprintui.h>
#include "print_gimp.h"

#include "print-intl.h"

void
do_gimp_install_procedure(const char *blurb, const char *help,
			  const char *auth, const char *copy,
			  const char *types, int n_args,
			  GimpParamDef *args)
{
  gimp_install_procedure (cast_safe("file_print_gutenprint"),
			  cast_safe(blurb),
			  cast_safe(help),
			  cast_safe(auth),
			  cast_safe(copy),
			  cast_safe(VERSION " - " RELEASE_DATE),
			  /* Do not translate the prefix "<Image>" */
#if (GIMP_MAJOR_VERSION > 2 || GIMP_MINOR_VERSION >= 2)
			  cast_safe(N_("_Print with Gutenprint...")),
#else
			  cast_safe(N_("<Image>/File/Print with Gutenprint...")),
#endif
			  cast_safe(types),
			  GIMP_PLUGIN,
			  n_args, 0,
			  args, NULL);
#if (GIMP_MAJOR_VERSION > 2 || GIMP_MINOR_VERSION >= 2)
  gimp_plugin_menu_register (cast_safe("file_print_gutenprint"),
			     cast_safe("<Image>/File/Send"));
  gimp_plugin_icon_register (cast_safe("file_print_gutenprint"),
                             GIMP_ICON_TYPE_STOCK_ID,
			     cast_safe(GTK_STOCK_PRINT));
#endif
}
