/*
 * "$Id$"
 *
 *   Dump the per-printer margins for Grant Taylor's *-omatic database
 *
 *   Copyright 2000, 2003 Robert Krawitz (rlk@alum.mit.edu) and
 *                        Till Kamppeter (till.kamppeter@gmx.net)
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <string.h>
#ifdef INCLUDE_GIMP_PRINT_H
#include INCLUDE_GIMP_PRINT_H
#else
#include <gimp-print/gimp-print.h>
#endif
#include "../../lib/libprintut.h"

int
main(int argc, char **argv) {
  int i, k;

  stp_init();
  for (i = 0; i < stp_known_printers(); i++) {
    const stp_printer_t p = stp_get_printer_by_index(i);
    const char *driver = stp_printer_get_driver(p);
    const char *family = stp_printer_get_family(p);
    stp_vars_t pv = 
      stp_allocate_copy(stp_printer_get_printvars(p));
    stp_parameter_t desc;
    int num_opts;
    const stp_param_string_t *opt;
    int width, height, bottom, left, top, right;
    if (strcmp(family, "ps") == 0 || strcmp(family, "raw") == 0)
      continue;
    printf("# Printer model %s, long name `%s'\n", driver,
	   stp_printer_get_long_name(p));
    stp_describe_parameter(pv, "PageSize", &desc);
    num_opts = stp_string_list_count(desc.bounds.str);
    
    for (k = 0; k < num_opts; k++) {
      stp_papersize_t papersize;
      opt = stp_string_list_param(desc.bounds.str, k);
      papersize = stp_get_papersize_by_name(opt->name);
      
      if (!papersize) {
	printf("Unable to lookup size %s!\n", opt->name);
	continue;
      }
      
      width  = stp_papersize_get_width(papersize);
      height = stp_papersize_get_height(papersize);
      
      stp_set_string_parameter(pv, "PageSize", opt->name);
      
      stp_get_media_size(pv, &width, &height);
      stp_get_imageable_area(pv, &left, &right, &bottom, &top);
      bottom = height - bottom;
      top    = height - top;

      if (strcmp(opt->name, "Custom") == 0) {
	/* Use relative values for the custom size */
	right = width - right;
	top = height - top;
	width = 0;
	height = 0;
      }

      printf("$imageableareas{'%s'}{'%s'} = {\n",
	     driver, opt->name);
      printf("  'left' => '%d',\n", left);
      printf("  'right' => '%d',\n", right);
      printf("  'top' => '%d',\n", top);
      printf("  'bottom' => '%d',\n", bottom);
      printf("  'width' => '%d',\n", width);
      printf("  'height' => '%d'\n", height);
      printf("};\n");
    }
    stp_free_parameter_description(&desc);
    stp_vars_free(pv);
  }
  return 0;
}
