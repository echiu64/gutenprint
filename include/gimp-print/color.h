/*
 * "$Id$"
 *
 *   libgimpprint color functions.
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
 * @file color.h
 * @brief Color functions.
 */

#ifndef GIMP_PRINT_COLOR_H
#define GIMP_PRINT_COLOR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The color data type is responsible for providing colour
 * conversion features.  Color modules provide the actual
 * functionality, so different colour management modules may provide
 * the application with different services (for example, colour
 * profiles).

 * @defgroup color color
 * @{
 */

/** The color opaque data type. */
typedef void *stp_color_t;
/** The constant color opaque data type. */
typedef const void *stp_const_color_t;

/**
 * Get the number of available color modules.
 * @returns the number of color modules.
 */
extern int
stp_color_count(void);

/**
 * Get a color module by its name.
 * @param name the short unique name.
 * number of papers - 1).
 * @returns a pointer to the color module, or NULL on failure.
 */
extern stp_const_color_t
stp_get_color_by_name(const char *name);

/**
 * Get a color module by its index number.
 * @param idx the index number.  This must not be greater than (total
 * number of papers - 1).
 * @returns a pointer to the color module, or NULL on failure.
 */
extern stp_const_color_t
stp_get_color_by_index(int idx);

/**
 * Get the short (untranslated) name of a color module.
 * @param c the color module to use.
 * @returns the short name.
 */
extern const char *
stp_color_get_name(stp_const_color_t c);

/**
 * Get the long (translated) name of a color module.
 * @param c the color module to use.
 * @returns the long name.
 */
extern const char *
stp_color_get_long_name(stp_const_color_t c);



#endif /* GIMP_PRINT_COLOR_H */
/*
 * End of "$Id$".
 */
