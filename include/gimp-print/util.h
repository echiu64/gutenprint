/*
 * "$Id$"
 *
 *   libgimpprint utility and miscellaneous functions.
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
 */


#ifndef __GIMP_PRINT_UTIL_H__
#define __GIMP_PRINT_UTIL_H__

#ifdef __cplusplus
extern "C" {
#endif


/*
 * stp_init() must be called prior to any other use of the library.
 */
extern int stp_init(void);


/*
 * Set the encoding that all translated strings are output in.
 */
extern const char *stp_set_output_codeset(const char *codeset);


#ifdef __cplusplus
  }
#endif

#endif /* __GIMP_PRINT_UTIL_H__ */
/*
 * End of "$Id$".
 */
