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


#ifndef GIMP_PRINT_COLOR_H
#define GIMP_PRINT_COLOR_H

#ifdef __cplusplus
extern "C" {
#endif


typedef void *stp_color_t;
typedef const void *stp_const_color_t;


extern int
stp_color_count(void);

extern stp_const_color_t
stp_get_color_by_index(int idx);

extern const char *
stp_color_get_name(stp_const_color_t c);

extern const char *
stp_color_get_long_name(stp_const_color_t c);

extern stp_const_vars_t
stp_color_get_defaults(stp_const_color_t c);

extern stp_const_color_t
stp_get_color_by_name(const char *name);


#endif /* GIMP_PRINT_COLOR_H */
/*
 * End of "$Id$".
 */
