/*
 * "$Id$"
 *
 *   libgimpprint printer functions.
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
 * @file printers.h
 * @brief Printer functions.
 */

#ifndef __GIMP_PRINT_PRINTERS_H__
#define __GIMP_PRINT_PRINTERS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <gimp-print/vars.h>

/**
 * The printer type represents a printer model.  A particular
 * printer model must selected in order to be able to print.  Each
 * printer model provides default print options through a default
 * vars object.
 *
 * @defgroup printer printer
 * @{
 */

/** The printer opaque data type (representation of printer model). */
typedef void *stp_printer_t;
/** The constant printer opaque data type (representation of printer model). */
typedef const void *stp_const_printer_t;

/**
 * Get the number of available printer models.
 * @returns the number of printer models.
 */
extern int stp_printer_model_count(void);

/**
 * Get a printer model by its index number.
 * @param idx the index number.  This must not be greater than (total
 * number of printers - 1).
 * @returns a pointer to the printer model, or NULL on failure.  The
 * pointer should not be freed.
 */
extern stp_const_printer_t stp_get_printer_by_index(int idx);

/**
 * Get a printer model by its long (translated) name.
 * @param long_name the printer model's long (translated) name.
 * @returns a pointer to the printer model, or NULL on failure.  The
 * pointer should not be freed.
 */
extern stp_const_printer_t stp_get_printer_by_long_name(const char *long_name);

/**
 * Get a printer model by its short name.
 * @param driver the printer model's short (driver) name.
 * @returns a pointer to the printer model, or NULL on failure.  The
 * pointer should not be freed.
 */
extern stp_const_printer_t stp_get_printer_by_driver(const char *driver);

/**
 * Get the printer model from a vars object.
 * @param v the vars to use.
 * @returns a pointer to the printer model, or NULL on failure.  The
 * pointer should not be freed.
 */
extern stp_const_printer_t stp_get_printer(stp_const_vars_t v);

/**
 * Get the printer index number from the printer model short (driver) name.
 * @deprecated there should never be any need to use this function.
 * @param driver the printer model's short (driver) name.
 * @returns the index number, or -1 on failure.
 */
extern int stp_get_printer_index_by_driver(const char *driver);

/**
 * Get a printer model's long (translated) name.
 * @param p the printer model to use.
 * @returns the long name (should never be freed).
 */
extern const char *stp_printer_get_long_name(stp_const_printer_t p);

/**
 * Get a printer model's short (driver) name.
 * @param p the printer model to use.
 * @returns the short name (should never be freed).
 */
extern const char *stp_printer_get_driver(stp_const_printer_t p);

/**
 * Get a printer model's family name.
 * The family name is the name of the modular "family" driver this
 * model uses.
 * @param p the printer model to use.
 * @returns the family name (should never be freed).
 */
extern const char *stp_printer_get_family(stp_const_printer_t p);

/**
 * Get a printer model's manufacturer's name.
 * @param p the printer model to use.
 * @returns the manufacturer's name (should never be freed).
 */
extern const char *stp_printer_get_manufacturer(stp_const_printer_t p);

/**
 * Get a printer model's model number.
 * The model number is used internally by the "family" driver module,
 * and has no meaning out of that context.  It bears no relation to
 * the model name/number actually found on the printer itself.
 * @param p the printer model to use.
 * @returns the model number.
 */
extern int stp_printer_get_model(stp_const_printer_t p);

/**
 * Get the default vars for a particular printer model.
 * The default vars should be copied to a new vars object and
 * customised prior to printing.
 * @param p the printer model to use.
 * @returns the printer model's default vars.
 */
extern stp_const_vars_t stp_printer_get_defaults(stp_const_printer_t p);

/**
 * Set a vars object to use a particular driver, and set the parameter
 * to its defaults.
 * @param v the vars to use.
 * @param p the printer model to use.
 */
extern void stp_set_printer_defaults(stp_vars_t v, stp_const_printer_t p);


/**
 * Print the image.
 * @warning stp_job_start() must be called prior to the first call to
 * this function.
 * @param v the vars to use.
 * @param image the image to print.
 * @returns 0 on failure, 1 on success, 2 on abort requested by the
 * driver.
 */
extern int stp_print(stp_const_vars_t v, stp_image_t *image);

/**
 * Start a print job.
 * @warning This function must be called prior to the first call to
 * stp_print().
 * @param v the vars to use.
 * @param image the image to print.
 * @returns 1 on success, 0 on failure.
 */
extern int stp_start_job(stp_const_vars_t v, stp_image_t *image);

/**
 * End a print job.
 * @param v the vars to use.
 * @param image the image to print.
 * @returns 1 on success, 0 on failure.
 */
extern int stp_end_job(stp_const_vars_t v, stp_image_t *image);

/** @} */

#ifdef __cplusplus
  }
#endif

#endif /* __GIMP_PRINT_PRINTERS_H__ */
/*
 * End of "$Id$".
 */
