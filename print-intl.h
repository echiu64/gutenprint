/*
 * "$Id$"
 *
 *   I18N header file for the gimp-print plugin.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and Michael Natterer (mitch@gimp.org)
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
#ifndef __PRINT_INTL_H__
#define __PRINT_INTL_H__

#include <glib.h>
#include <locale.h>

#ifndef LOCALEDIR
#define LOCALEDIR g_strconcat (gimp_data_directory (), \
			       G_DIR_SEPARATOR_S, \
			       "locale", \
			       NULL)
#endif

#ifdef ENABLE_NLS
#    include <libintl.h>
#    define _(String) gettext (String)
#    ifdef gettext_noop
#        define N_(String) gettext_noop (String)
#    else
#        define N_(String) (String)
#    endif
#else
/* Stubs that do something close enough.  */
#    define textdomain(String) (String)
#    define gettext(String) (String)
#    define dgettext(Domain,Message) (Message)
#    define dcgettext(Domain,Message,Type) (Message)
#    define bindtextdomain(Domain,Directory) (Domain)
#    define _(String) (String)
#    define N_(String) (String)
#endif

#define INIT_LOCALE(domain)   G_STMT_START{ \
        gtk_set_locale ();                  \
        setlocale (LC_NUMERIC, "C");        \
        bindtextdomain (domain, LOCALEDIR); \
        textdomain (domain);                \
                                }G_STMT_END

#endif /* __PRINT_INTL_H__ */
