/*
 *
 *   Find the smallest page size for a given printer
 *
 *   Copyright 2018 Robert Krawitz (rlk@alum.mit.edu)
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <string.h>
#include <gutenprint/gutenprint.h>
#include <gutenprint/gutenprint-intl.h>


int
main(int argc, char **argv)
{
  if (argc != 2)
    {
      fprintf(stderr, "Usage: %s printer\n", argv[0]);
      exit(1);
    }
  stp_init();
  const stp_printer_t *printer = stp_get_printer_by_driver(argv[1]);
  if (! printer)
    {
      fprintf(stderr, "%s: printer %s not found\n", argv[0], argv[1]);
      exit(1);
    }
  stp_dimension_t min_area = 7200 * 7200;
  const char *min_size_name = NULL;
  int i;
  stp_vars_t *pv = stp_vars_create_copy(stp_printer_get_defaults(printer));
  stp_parameter_t desc;
  stp_describe_parameter(pv, "PageSize", &desc);
  if (!desc.is_active || desc.p_type != STP_PARAMETER_TYPE_STRING_LIST)
    {
      fprintf(stderr, "%s: error getting page size list for printer %s\n",
	      argv[0], argv[1]);
      exit(1);
    }
  size_t count = stp_string_list_count(desc.bounds.str);
  if (count > 0)
    {
      for (i = 0; i < count; i++)
	{
	  const stp_param_string_t *param = 
	    stp_string_list_param(desc.bounds.str, i);
	  const stp_papersize_t *papersize = 
	    stp_describe_papersize(pv, param->name);
	  if (papersize->width > 0 && papersize->height > 0 &&
	      (papersize->width * papersize->height) < min_area)
	    {
	      min_area = papersize->width * papersize->height;
	      min_size_name = param->name;
	    }
	}
    }
  if (min_size_name)
    {
      puts(min_size_name);
      exit(0);
    }
  else
    {
      fprintf(stderr, "%s: cannot find smallest page size for printer %s\n",
	      argv[0], argv[1]);
      exit(1);
    }
}

