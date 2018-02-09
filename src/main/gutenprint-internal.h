/*
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
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Revision History:
 *
 *   See ChangeLog
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifndef GUTENPRINT_INTERNAL_INTERNAL_H
#define GUTENPRINT_INTERNAL_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gutenprint/gutenprint-module.h>
#include <time.h>

/**
 * Utility functions (internal).
 *
 * @defgroup util_internal util-internal
 * @{
 */

extern void stpi_init_paper(void);
extern void stpi_init_dither(void);
extern void stpi_init_printer(void);
#define BUFFER_FLAG_FLIP_X	0x1
#define BUFFER_FLAG_FLIP_Y	0x2
extern stp_image_t* stpi_buffer_image(stp_image_t* image, unsigned int flags);

#define STPI_ASSERT(x,v)						\
do									\
{									\
  if (stp_get_debug_level() & STP_DBG_ASSERTIONS)			\
    stp_erprintf("DEBUG: Testing assertion %s file %s line %d\n",	\
		 #x, __FILE__, __LINE__);				\
  if (!(x))								\
    {									\
      stp_erprintf("\nERROR: ***Gutenprint %s assertion %s failed!"	\
		   " file %s, line %d.  %s\n", PACKAGE_VERSION,		\
		   #x, __FILE__, __LINE__, "Please report this bug!");	\
      if ((v)) stp_vars_print_error((v), "ERROR");			\
      stp_abort();							\
    }									\
} while (0)

/** @} */

/* Internal printer stuff, moved from printers.h */
typedef struct
{
  stp_parameter_list_t (*list_parameters)(const stp_vars_t *v);
  void  (*parameters)(const stp_vars_t *v, const char *name,
		      stp_parameter_t *);
  void  (*media_size)(const stp_vars_t *v, stp_dimension_t *width,
		      stp_dimension_t *height);
  void  (*imageable_area)(const stp_vars_t *v, stp_dimension_t *left,
			  stp_dimension_t *right, stp_dimension_t *bottom,
			  stp_dimension_t *top);
  void  (*maximum_imageable_area)(const stp_vars_t *v, stp_dimension_t *left,
				  stp_dimension_t *right, stp_dimension_t *bottom,
				  stp_dimension_t *top);
  void  (*limit)(const stp_vars_t *v, stp_dimension_t *max_width,
		 stp_dimension_t *max_height, stp_dimension_t *min_width,
		 stp_dimension_t *min_height);
  int   (*print)(const stp_vars_t *v, stp_image_t *image);
  void  (*describe_resolution)(const stp_vars_t *v, stp_resolution_t *x,
			       stp_resolution_t *y);
  const char *(*describe_output)(const stp_vars_t *v);
  int   (*verify)(stp_vars_t *v);
  int   (*start_job)(const stp_vars_t *v, stp_image_t *image);
  int   (*end_job)(const stp_vars_t *v, stp_image_t *image);
  stp_string_list_t *(*get_external_options)(const stp_vars_t *v);
  const stp_papersize_t *(*describe_papersize)(const stp_vars_t *v,
					       const char *name);
} stp_printfuncs_t;

typedef struct stp_family
{
  const stp_printfuncs_t *printfuncs;   /* printfuncs for the printer */
  stp_list_t             *printer_list; /* list of printers */
} stp_family_t;

extern int stpi_family_register(stp_list_t *family);
extern int stpi_family_unregister(stp_list_t *family);


/*
 * Paper size functions
 */

typedef stp_list_t stp_papersize_list_t;
typedef stp_list_item_t stp_papersize_list_item_t;
#define stpi_papersize_list_get_start stp_list_get_start
#define stpi_papersize_list_get_end stp_list_get_end
#define stpi_paperlist_item_next stp_list_item_next
#define stpi_paperlist_item_prev stp_list_item_prev
#define stpi_paperlist_item_get_data(item) (stp_papersize_t *) (stp_list_item_get_data((item)))

/**
 * Get a named list of paper sizes
 * @param name the list of paper sizes to find
 * @param file name of the file to load (relative to $STP_XML_PATH)
 * if the list does not exist.  Empty filename indicates that the system
 * should identify the file; NULL indicates that the list should not be
 * created if it does not exist.
 * @returns a static pointer to the papersize list, or NULL on failure
 */
extern const stp_papersize_list_t *stpi_get_papersize_list_named(const char *name,
								 const char *file);

/**
 * Create a new list of paper sizes without loading from a file.
 * @param name the list of paper sizes to create
 * @returns a static pointer to the (mutable) papersize list,
 * or NULL if the list already exists
 */
extern stp_papersize_list_t *stpi_new_papersize_list(const char *name);

/**
 * Find an existing papersize list, if it exists
 * @param name the list of paper sizes to find
 * @returns a static pointer to the (mutable) papersize list,
 * or NULL if the list does not exist
 */
extern stp_papersize_list_t *stpi_find_papersize_list_named(const char *name);

/**
 * Get the standard papersize list.
 * @returns a static const pointer to the standard paper list.
 */
extern const stp_papersize_list_t *stpi_get_standard_papersize_list(void);

/**
 * Create and return a new paper list
 * @returns a pointer to the new paper list
 */
extern stp_papersize_list_t *stpi_create_papersize_list(void);

/**
 * Get a papersize by its name from a list of papersizes  Paper sizes
 * @param list the list of papers to search
 * @param name the name of the paper to search for
 * @returns a static pointer to the papersize, or NULL on failure.
 */
extern const stp_papersize_t *stpi_get_papersize_by_name(const stp_papersize_list_t *list,
							 const char *name);

/**
 * Dispatch to printer-specific call to describe paper size
 * @param v the Gutenprint vars object
 * @param name the name of the paper size
 * @returns a static pointer to the papersize, or NULL on failure.
 */
extern const stp_papersize_t *stpi_printer_describe_papersize(const stp_vars_t *v,
							      const char *name);


/**
 * Get a papersize by its name from the standard list of papersizes
 * @param v the Gutenprint vars object
 * @param name the name of the paper size
 * @returns a static pointer to the papersize, or NULL on failure.
 */
extern const stp_papersize_t *stpi_standard_describe_papersize(const stp_vars_t *v,
							       const char *name);

/**
 * Add a new papersize to a list
 * @param list the name of the list to search
 * @param papersize the stp_papersize_t to add
 * @returns 1 on success, 0 on failure (e. g. already exists)
 */
extern int stpi_papersize_create(stp_papersize_list_t *list,
				 stp_papersize_t *p);

/**
 * Get a papersize by its name from a list
 * @param list the name of the list to search
 * @param name the name of the paper size
 * @returns a static pointer to the papersize, or NULL on failure.
 */
extern const stp_papersize_t *stpi_get_listed_papersize(const char *list,
							const char *name);

/**
 * Get the number of available papersizes.
 * @param list the paper size list
 * @returns the number of papersizes.
 */
extern int stpi_papersize_count(const stp_list_t *paper_size_list);

/**
 * Get a papersize by size.
 * The nearest available size to the size requested will be found.
 * Only paper sizes within 5 points of width and height will be considered.
 * @param v the Gutenprint vars object
 * @param length the length of the paper.
 * @param width the width of the paper
 * @returns a static pointer to the papersize, or NULL on failure.
 */
extern const stp_papersize_t *stpi_get_papersize_by_size(const stp_papersize_list_t *l,
							 stp_dimension_t length,
							 stp_dimension_t width);

/**
 * Get a papersize by size if an exact match is found.
 * @param v the Gutenprint vars object
 * @param length the length of the paper.
 * @param width the width of the paper
 * @returns a static pointer to the papersize, or NULL on failure.
 */
extern const stp_papersize_t *stpi_get_papersize_by_size_exact(const stp_papersize_list_t *l,
							       stp_dimension_t length,
							       stp_dimension_t width);

/**
 * Check for duplicate printers.  Abort if any duplicates are found.
 */
extern void stpi_find_duplicate_printers(void);

extern time_t stpi_time(time_t *t);

#define CAST_IS_SAFE GCC_DIAG_OFF(cast-qual)
#define CAST_IS_UNSAFE GCC_DIAG_ON(cast-qual)

#pragma GCC diagnostic ignored "-Woverlength-strings"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
static inline void *
stpi_cast_safe(const void *ptr)
{
  return (void *)ptr;
}
#pragma GCC diagnostic pop

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_INTERNAL_INTERNAL_H */
