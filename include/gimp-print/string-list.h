/*
 * "$Id$"
 *
 *   libgimpprint string list functions.
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


#ifndef __GIMP_PRINT_STRING_LIST_H__
#define __GIMP_PRINT_STRING_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif


/****************************************************************
*                                                               *
* LISTS OF STRINGS                                              *
*                                                               *
****************************************************************/

extern stp_string_list_t stp_string_list_create(void);
extern void stp_string_list_free(stp_string_list_t list);

extern stp_param_string_t *stp_string_list_param(stp_const_string_list_t,
						 size_t element);

extern size_t stp_string_list_count(stp_const_string_list_t list);

extern stp_string_list_t stp_string_list_create_copy(stp_const_string_list_t);

extern void stp_string_list_add_string(stp_string_list_t list,
				       const char *name, const char *text);

extern stp_string_list_t
stp_string_list_create_from_params(const stp_param_string_t *list,
				   size_t count);

extern int
stp_string_list_is_present(stp_string_list_t list, const char *value);


#ifdef __cplusplus
  }
#endif

#endif /* __GIMP_PRINT_STRING_LIST_H__ */
/*
 * End of "$Id$".
 */
