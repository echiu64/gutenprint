/*
 *   libgimpprint path functions header
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and Michael Natterer (mitch@gimp.org)
 *   Copyright 2002 Roger Leigh (rleigh@debian.org)
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

/**
 * @file gutenprint/path.h
 * @brief Simple directory path functions.
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifndef GUTENPRINT_PATH_H
#define GUTENPRINT_PATH_H

#ifdef __cplusplus
extern "C" {
#endif


extern stp_list_t *stp_path_search(stp_list_t *dirlist,
				   const char *suffix);

extern void stp_path_split(stp_list_t *list,
			   const char *path);

/*
 * Split a path (colon-separated list of strings) into
 * its components.  The components may be anything that does not contain
 * a token.
 */
extern stp_list_t *stp_generate_path(const char *path);

extern stp_list_t *stp_data_path(void);

extern stp_list_t *stpi_list_files_on_data_path(const char *name);

/*
 * Join a path and filename together.
 */
extern char *stpi_path_merge(const char *path, const char *file);

/*
 * Find the first occurrence of <file> on <path>.
 * File must be a plain file and readable.
 * Return value must be freed
 */
extern char *stp_path_find_file(const char *path, const char *file);

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_PATH_H */
