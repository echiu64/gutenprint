/*
 * "$Id$"
 *
 *   libgimpprint module loader header
 *
 *   Copyright 1997-2002 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and Michael Natterer (mitch@gimp.org)
 *   Copyright 2002-2003 Roger Leigh (roger@whinlatter.uklinux.net)
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


#ifndef GIMP_PRINT_XML_H
#define GIMP_PRINT_XML_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libxml/tree.h>
#include <gimp-print/array.h>
#include <gimp-print/curve.h>

extern stp_curve_t stp_curve_create_from_file(const char* file);
extern stp_curve_t stp_curve_create_from_xmltree(xmlNodePtr curve);
extern stp_curve_t stp_curve_create_from_file(const char* file);
extern stp_curve_t stp_curve_create_from_string(const char* string);
extern xmlNodePtr stp_xmltree_create_from_curve(stp_curve_t curve);
extern int stp_curve_write(FILE *file, stp_curve_t curve);
extern xmlChar *stp_curve_write_string(stp_curve_t curve);

#endif /* GIMP_PRINT_XML_H */
/*
 * End of "$Id$".
 */
