/*
 * "$Id$"
 *
 *   Print plug-in header file for the GIMP.
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
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Revision History:
 *
 *   See ChangeLog
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifndef GIMP_PRINT_INTERNAL_INTERNAL_H
#define GIMP_PRINT_INTERNAL_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif


#define COOKIE_OPTION     0x3ab27f93
#define COOKIE_PARAM_LIST 0x96cf0387
#define COOKIE_PRINTER    0x0722922c


#include "color.h"
#include "dither.h"
#include "dither-matrices.h"
#include "papers.h"
#include "printers.h"
#include "util.h"
#include "vars.h"
#include "weave.h"


#ifdef __cplusplus
  }
#endif

#endif /* GIMP_PRINT_INTERNAL_INTERNAL_H */
/*
 * End of "$Id$".
 */
