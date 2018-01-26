/*
 *
 *   Dump the per-printer options for Gutenprint
 *
 *   Copyright 2000-2018 Robert Krawitz (rlk@alum.mit.edu)
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
  int i, j, k;
  int first_arg = 1;
  stp_string_list_t *printer_list = NULL;
  stp_parameter_level_t max_level = STP_PARAMETER_LEVEL_ADVANCED4;
  if (argc > 1 && !strcmp(argv[1], "-s"))
    {
      max_level = STP_PARAMETER_LEVEL_BASIC;
      first_arg++;
    }

  stp_init();

  if (argc > first_arg)
    {
      printer_list = stp_string_list_create();
      for (i = 1; i < argc; i++)
	stp_string_list_add_string(printer_list, argv[i], argv[i]);
    }
  for (i = 0; i < stp_printer_model_count(); i++)
    {
      stp_parameter_list_t params;
      int nparams;
      stp_parameter_t desc;
      const stp_printer_t *printer = stp_get_printer_by_index(i);
      const char *driver = stp_printer_get_driver(printer);
      const char *family = stp_printer_get_family(printer);
      stp_vars_t *pv;
      int tcount = 0;
      size_t count;
      int printer_is_color = 0;
      if (strcmp(family, "ps") == 0 || strcmp(family, "raw") == 0)
	continue;
      if (printer_list && !stp_string_list_is_present(printer_list, driver))
	continue;

      pv = stp_vars_create_copy(stp_printer_get_defaults(printer));
      /* Set Job Mode to "Job" as this enables the Duplex option */
      stp_set_string_parameter(pv, "JobMode", "Job");

      stp_describe_parameter(pv, "PrintingMode", &desc);
      if (stp_string_list_is_present(desc.bounds.str, "Color"))
	printer_is_color = 1;
      stp_parameter_description_destroy(&desc);
      if (printer_is_color)
	stp_set_string_parameter(pv, "PrintingMode", "Color");
      else
	stp_set_string_parameter(pv, "PrintingMode", "BW");
      stp_set_string_parameter(pv, "ChannelBitDepth", "8");

      printf("# Printer model %s, long name `%s'\n", driver,
	     stp_printer_get_long_name(printer));

      printf("$families{'%s'} = '%s';\n", driver, family);
      printf("$models{'%s'} = '%d';\n", driver, stp_get_model_id(pv));

      params = stp_get_parameter_list(pv);
      nparams = stp_parameter_list_count(params);

      for (k = 0; k < nparams; k++)
	{
	  const stp_parameter_t *p = stp_parameter_list_param(params, k);
	  if (p->read_only ||
	      (p->p_level > max_level && strcmp(p->name, "Resolution") != 0) ||
	      (p->p_class != STP_PARAMETER_CLASS_OUTPUT &&
	       p->p_class != STP_PARAMETER_CLASS_CORE &&
	       p->p_class != STP_PARAMETER_CLASS_FEATURE))
	    continue;
	  count = 0;
	  stp_describe_parameter(pv, p->name, &desc);
	  if (desc.is_active)
	    {
	      if (desc.p_type == STP_PARAMETER_TYPE_STRING_LIST)
		{
		  count = stp_string_list_count(desc.bounds.str);
		  if (count > 0)
		    {
		      if (strcmp(desc.name, "Resolution") == 0)
			{
			  for (j = 0; j < count; j++)
			    {
			      const stp_param_string_t *param =
				stp_string_list_param(desc.bounds.str, j);
			      stp_resolution_t x, y;
			      stp_set_string_parameter(pv, "Resolution",
						       param->name);
			      stp_describe_resolution(pv, &x, &y);
			      if (x > 0 && y > 0)
				printf("$resolutions{'%s'}{'%s'} = [%d, %d];\n",
				       driver, param->name, (int) x, (int) y);
			    }
			  stp_clear_string_parameter(pv, "Resolution");
			}
		      else
			{
			  if (strcmp(desc.name, "PageSize") == 0)
			    {
			      stp_dimension_t min_area = 7200 * 7200;
			      stp_dimension_t max_area = 0;
			      const char *min_size_name = NULL;
			      const char *max_size_name = NULL;
			      for (j = 0; j < count; j++)
				{
				  const stp_param_string_t *param =
				    stp_string_list_param(desc.bounds.str, j);
				  const stp_papersize_t *ps =
				    stp_describe_papersize(pv, param->name);
				  if (ps->width > 0 && ps->height > 0 &&
				      (ps->width * ps->height) < min_area)
				    {
				      min_area = ps->width * ps->height;
				      min_size_name = param->name;
				    }
				  if (ps->width > 0 && ps->height > 0 &&
				      (ps->width * ps->height) > max_area)
				    {
				      max_area = ps->width * ps->height;
				      max_size_name = param->name;
				    }
				}
			      if (min_size_name)
				{
				  printf("$min_page_size{'%s'} = '%s';\n",
					 driver, min_size_name);
				  printf("$max_page_size{'%s'} = '%s';\n",
					 driver, max_size_name);
				}
			    }
			  printf("$stpdata{'%s'}{'%s'} = [qw(", driver, desc.name);
			  if (!desc.is_mandatory && !
			      stp_string_list_is_present(desc.bounds.str, "None"))
			    fputs("+None ", stdout);
			  for (j = 0; j < count; j++)
			    {
			      const stp_param_string_t *param =
				stp_string_list_param(desc.bounds.str, j);
			      printf("%s%s ",
				     (strcmp(desc.deflt.str, param->name)) ? "" : "+",
				     param->name);
			    }
			  fputs(")];\n", stdout);
			}
		    }
		}
	      else if (desc.p_type == STP_PARAMETER_TYPE_BOOLEAN)
		{
		  if (desc.is_mandatory)
		    printf("$stp_bools{'%s'}{'%s'} = %d;\n",
			   driver, desc.name, desc.deflt.boolean);
		  else
		    printf("$stp_bools{'%s'}{'%s'} = %d;\n",
			   driver, desc.name, -1);
		}
	      else if (desc.p_type == STP_PARAMETER_TYPE_DOUBLE)
		{
		  if (desc.bounds.dbl.lower <= desc.deflt.dbl &&
		      desc.bounds.dbl.upper >= desc.deflt.dbl)
		    printf("$stp_float_values{'%s'}{'%s'} = [%d, %.3f, %.3f, %.3f];\n",
			   driver, desc.name, desc.is_mandatory,
			   desc.deflt.dbl, desc.bounds.dbl.lower,
			   desc.bounds.dbl.upper);
		}
	      else if (desc.p_type == STP_PARAMETER_TYPE_INT)
		{
		  if (desc.bounds.integer.lower <= desc.deflt.integer &&
		      desc.bounds.integer.upper >= desc.deflt.integer)
		    printf("$stp_int_values{'%s'}{'%s'} = [%d, %d, %d, %d];\n",
			   driver, desc.name, desc.is_mandatory,
			   desc.deflt.integer, desc.bounds.integer.lower,
			   desc.bounds.integer.upper);
		}
	      else if (desc.p_type == STP_PARAMETER_TYPE_DIMENSION)
		{
		  if (desc.bounds.dimension.lower <= desc.deflt.dimension &&
		      desc.bounds.dimension.upper >= desc.deflt.dimension)
		    printf("$stp_dimension_values{'%s'}{'%s'} = [%d, %.3f, %.3f, %.3f];\n",
			   driver, desc.name, desc.is_mandatory,
			   desc.deflt.dimension, desc.bounds.dimension.lower,
			   desc.bounds.dimension.upper);
		}
	      tcount += count;
	    }
	  stp_parameter_description_destroy(&desc);
	}
      stp_parameter_list_destroy(params);
      if (tcount > 0)
	{
	  printf("$stpdata{'%s'}{'Color'} = [qw(", driver);
	  if (printer_is_color)
	    fputs("Color RawCMYK ", stdout);
	  fputs("Grayscale BlackAndWhite)];\n", stdout);
	}
      stp_vars_destroy(pv);
    }
  if (printer_list)
    stp_string_list_destroy(printer_list);
  return 0;
}
