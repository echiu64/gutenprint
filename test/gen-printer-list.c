/*
 *
 *   Test pattern generator for Gutenprint
 *
 *   Copyright 2014 Robert Krawitz <rlk@alum.mit.edu>
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

/*
 * Generate list of printers for supported printer list
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <gutenprint/gutenprint.h>
#include <string.h>

int
main(int argc, char **argv)
{
  int i;

  stp_init();
  for (i = 0; i < stp_printer_model_count(); i++)
    {
      const stp_printer_t *p = stp_get_printer_by_index(i);
      if (strcmp(stp_printer_get_family(p), "ps") &&
	  strcmp(stp_printer_get_family(p), "raw"))
	printf("<tr><td>%s</td><td>%s</td><td>%s</td></tr>\n",
	       stp_printer_get_long_name(p),
	       stp_printer_get_driver(p),
	       stp_printer_get_comment(p) ? stp_printer_get_comment(p) : "");
    }
  return 0;
}
