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


#ifndef __GIMP_PRINT_PRINTERS_H__
#define __GIMP_PRINT_PRINTERS_H__

#ifdef __cplusplus
extern "C" {
#endif


#include <gimp-print/vars.h>

/*
 * Opaque representation of a printer model
 */

typedef void *stp_printer_t;
typedef const void *stp_const_printer_t;


/****************************************************************
*                                                               *
* PRINTER DESCRIPTION                                           *
*                                                               *
****************************************************************/

extern int stp_printer_model_count(void);
extern stp_const_printer_t stp_get_printer_by_index(int idx);
extern stp_const_printer_t stp_get_printer_by_long_name(const char *long_name);
extern stp_const_printer_t stp_get_printer_by_driver(const char *driver);
extern stp_const_printer_t stp_get_printer(stp_const_vars_t v);
extern int stp_get_printer_index_by_driver(const char *driver);

extern const char *stp_printer_get_long_name(stp_const_printer_t p);
extern const char *stp_printer_get_driver(stp_const_printer_t p);
extern const char *stp_printer_get_family(stp_const_printer_t p);
extern const char *stp_printer_get_manufacturer(stp_const_printer_t p);
extern int stp_printer_get_model(stp_const_printer_t p);
extern stp_const_vars_t stp_printer_get_defaults(stp_const_printer_t p);
extern void stp_set_printer_defaults(stp_vars_t, stp_const_printer_t);


/*
 * Actually print the image.  Return value of 0 represents failure; status of 1
 * represents success; status of 2 represents abort requested by the driver.
 */
extern int stp_print(stp_const_vars_t v, stp_image_t *image);

/*
 * Must be called prior to the first call to stp_print().
 */
extern int stp_start_job(stp_const_vars_t, stp_image_t *image);

extern int stp_end_job(stp_const_vars_t, stp_image_t *image);


#ifdef __cplusplus
  }
#endif

#endif /* __GIMP_PRINT_PRINTERS_H__ */
/*
 * End of "$Id$".
 */
