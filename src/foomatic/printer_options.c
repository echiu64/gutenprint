/*
 * "$Id$"
 *
 *   Dump the per-printer options for Grant Taylor's *-omatic database
 *
 *   Copyright 2000 Robert Krawitz (rlk@alum.mit.edu)
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
#ifdef INCLUDE_GIMP_PRINT_H
#include INCLUDE_GIMP_PRINT_H
#else
#include <gimp-print/gimp-print.h>
#endif
#include "../../lib/libprintut.h"

const char *params[] =
{
  "PageSize",
  "Resolution",
  "InkType",
  "MediaType",
  "InputSlot",
  "DitherAlgorithm"
};

int nparams = sizeof(params) / sizeof(const char *);

int
main(int argc, char **argv)
{
  int i, j, k;

  stp_init();
  for (i = 0; i < stp_known_printers(); i++)
    {
      const stp_printer_t p = stp_get_printer_by_index(i);
      stp_vars_t pv = stp_allocate_copy(stp_printer_get_printvars(p));
      int tcount = 0;
      printf("# Printer model %s, long name `%s'\n",
	     stp_printer_get_driver(p), stp_printer_get_long_name(p));
      for (k = 0; k < nparams; k++)
	{
	  stp_param_list_t *retval =
	    stp_printer_get_parameters(p, pv, params[k]);
	  const char *retval1 =
	    stp_printer_get_default_parameter(p, pv, params[k]);
	  size_t count = stp_param_list_count(retval);
	  if (count > 0)
	    {
	      printf("$defaults{'%s'}{'%s'} = '%s';\n",
		     stp_printer_get_driver(p), params[k], retval1);
	      for (j = 0; j < count; j++)
		{
		  const stp_param_t *param = stp_param_list_param(retval, j);
		  printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';\n",
			 stp_printer_get_driver(p), params[k],
			 param->name, param->text);
		  if (strcmp(params[k], "Resolution") == 0)
		    {
		      int x, y;
		      stp_set_parameter(pv, "Resolution", param->name);
		      stp_printer_describe_resolution(p, pv, &x, &y);
		      if (x > 0 && y > 0)
			{
			  printf("$stpdata{'%s'}{'%s'}{'%s'} = '%d';\n",
				 stp_printer_get_driver(p), "x_resolution",
				 param->name, x);
			  printf("$stpdata{'%s'}{'%s'}{'%s'} = '%d';\n",
				 stp_printer_get_driver(p), "y_resolution",
				 param->name, y);
			}
		    }
		}
	    }
	  stp_param_list_free(retval);
	  tcount += count;
	}
      if (tcount > 0)
	{
	  if (stp_get_output_type(pv) == OUTPUT_COLOR)
	    {
	      printf("$defaults{'%s'}{'%s'} = '%s';\n",
		     stp_printer_get_driver(p), "Color", "Color");
	      printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';\n",
		     stp_printer_get_driver(p), "Color", "Color",
		     "Color");
	      printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';\n",
		     stp_printer_get_driver(p), "Color", "RawCMYK",
		     "Raw CMYK");
	    }
	  else
	    printf("$defaults{'%s'}{'%s'} = '%s';\n",
		   stp_printer_get_driver(p), "Color", "Grayscale");
	  printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';\n",
		 stp_printer_get_driver(p), "Color", "Grayscale",
		 "Gray Scale");
	  printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';\n",
		 stp_printer_get_driver(p), "Color", "BlackAndWhite",
		 "Black and White");
	}
      stp_free_vars(pv);
    }
  return 0;
}
