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

int
main(int argc, char **argv)
{
  stp_parameter_t desc;
  int nparams;
  int i;
  const stp_parameter_t *params;

  stp_init();
  params = stp_list_parameters(stp_default_settings(), &nparams);
  for (i = 0; i < nparams; i++)
    {
      if (params[i].type == STP_PARAMETER_TYPE_DOUBLE)
	{
	  stp_describe_parameter(stp_default_settings(),
				 params[i].name, &desc);
	  printf("$stp_values{'MINVAL'}{'%s'} = %.3f\n",
		 params[i].name, desc.bounds.dbl.lower);
	  printf("$stp_values{'MAXVAL'}{'%s'} = %.3f\n",
		 params[i].name, desc.bounds.dbl.upper);
	  printf("$stp_values{'DEFVAL'}{'%s'} = %.3f\n",
		 params[i].name, desc.deflt.dbl);
	}
      else if (params[i].type == STP_PARAMETER_TYPE_INT)
	{
	  stp_describe_parameter(stp_default_settings(),
				 params[i].name, &desc);
	  printf("$stp_values{'MINVAL'}{'%s'} = %d\n",
		 params[i].name, desc.bounds.integer.lower);
	  printf("$stp_values{'MAXVAL'}{'%s'} = %d\n",
		 params[i].name, desc.bounds.integer.upper);
	  printf("$stp_values{'DEFVAL'}{'%s'} = %d\n",
		 params[i].name, desc.deflt.integer);
	}
    }
  return 0;
}
