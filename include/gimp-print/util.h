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

/**
 * @file util.h
 * @brief Utility functions.
 */

#ifndef __GIMP_PRINT_UTIL_H__
#define __GIMP_PRINT_UTIL_H__

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Utility functions.
 *
 * @defgroup util util
 * @{
 */

/**
 * Initialise libgimpprint.
 * This function must be called prior to any other use of the library.
 * It is responsible for loading modules and XML data and initialising
 * internal data structures.
 * @returns 0 on success, 1 on failure.
 */
extern int stp_init(void);


/**
 * Set the output encoding.  This function sets the encoding that all
 * strings translated by gettext are output in.  It is a wrapper
 * around the gettext bind_textdomain_codeset() function.
 * @param codeset the standard name of the encoding, which must be
 * usable with iconv_open().  For example, "US-ASCII" or "UTF-8".  If
 * NULL, the currently-selected codeset will be returned (or NULL if
 * no codeset has been selected yet).
 * @returns a string containing the selected codeset, or NULL on
 * failure (errno is set accordingly).
 */
extern const char *stp_set_output_codeset(const char *codeset);

/** @} */

#ifdef __cplusplus
  }
#endif

#endif /* __GIMP_PRINT_UTIL_H__ */
/*
 * End of "$Id$".
 */
