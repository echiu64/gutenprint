/*
 *   libgimpprint paper functions.
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

/**
 * @file gutenprint/paper.h
 * @brief Paper size functions.
 */

#ifndef GUTENPRINT_PAPER_H
#define GUTENPRINT_PAPER_H

#include <gutenprint/types.h>
#include <gutenprint/vars.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The papersize describes the dimensions of a paper.
 *
 * @defgroup papersize papersize
 * @{
 */



/**
 * Units of measurement.
 */
typedef enum
{
  /** English/Imperial units. */
  PAPERSIZE_ENGLISH_STANDARD,
  /** Metric units. */
  PAPERSIZE_METRIC_STANDARD,
  /** English/Imperial units (optional paper, not displayed by default). */
  PAPERSIZE_ENGLISH_EXTENDED,
  /** Metric units (optional paper, not displayed by default). */
  PAPERSIZE_METRIC_EXTENDED
} stp_papersize_unit_t;

typedef enum
{
  /** Standard paper size */
  PAPERSIZE_TYPE_STANDARD = 0,
  /** Envelope */
  PAPERSIZE_TYPE_ENVELOPE
} stp_papersize_type_t;

/** The papersize data type. */
typedef struct
{
  /** Short unique name (not translated). */
  const char *name;
  /** Long descriptive name (translated). */
  const char *text;
  /** Comment. */
  const char *comment;
  /** Paper width. */
  stp_dimension_t width;
  /** Paper height. */
  stp_dimension_t height;
  /** Top margin. */
  stp_dimension_t top;
  /** Left margin. */
  stp_dimension_t left;
  /** Bottom margin. */
  stp_dimension_t bottom;
  /** Right margin. */
  stp_dimension_t right;
  /** Units of measurement. */
  stp_papersize_unit_t paper_unit;
  /** Paper size type. */
  stp_papersize_type_t paper_size_type;
} stp_papersize_t;

/**
 * Get a papersize by name.
 * @param v the Gutenprint vars object
 * @param name the short unique name of the paper.
 * @returns a static pointer to the papersize, or NULL on failure.
 */
extern const stp_papersize_t *stp_describe_papersize(const stp_vars_t *v,
						     const char *name);

/**
 * Get the default paper dimensions for the current configuration.
 * The default is derived from the PageSize parameter if set, otherwise
 * the default page size for the printer is used.  If no value can be
 * determined, 1x1 will be returned.
 * @param v the Gutenprint vars object
 * @param width pointer to storage that the width will be returned in.
 * @param height pointer to storage that the height will be returned in.
 */
extern void stp_default_media_size(const stp_vars_t *v,
				   stp_dimension_t *width, stp_dimension_t *height);

/** @} */

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_PAPER_H */
