/*
 * "$Id$"
 *
 *   Print plug-in driver utility functions for the GIMP.
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


#ifndef GIMP_PRINT_PRINTLIST_H
#define GIMP_PRINT_PRINTLIST_H

typedef struct stp_old_printer
{
  const char *long_name;	/* Long name for UI */
  const char *short_name;
  int        model;		/* Model number */
  int	     color;
  const stp_printfuncs_t *printfuncs;
  const char *printer_data;
} stp_old_printer_t;

#endif /* GIMP_PRINT_PRINTLIST_H */
/*
 * End of "$Id$".
 */
