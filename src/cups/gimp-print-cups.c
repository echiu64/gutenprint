/*
 * "$Id$"
 *
 *   PPD file generation program for the CUPS drivers.
 *
 *   Copyright 1993-2002 by Easy Software Products.
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License,
 *   version 2, as published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, please contact Easy Software
 *   Products at:
 *
 *       Attn: CUPS Licensing Information
 *       Easy Software Products
 *       44141 Airport View Drive, Suite 204
 *       Hollywood, Maryland 20636-3111 USA
 *
 *       Voice: (301) 373-9603
 *       EMail: cups-info@cups.org
 *         WWW: http://www.cups.org
 *
 * Contents:
 *
 *   main()                   - Process files on the command-line...
 *   initialize_stp_options() - Initialize the min/max values for
 *                              each STP numeric option.
 *   usage()                  - Show program usage.
 *   help()                   - Show detailed program usage.
 *   getlangs()               - Get available translations.
 *   printlangs()             - Show available translations.
 *   printmodels()            - Show available printer models.
 *   checkcat()               - Check message catalogue exists.
 *   xmalloc()                - Die gracefully if malloc fails.
 *   write_ppd()              - Write a PPD file.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef INCLUDE_GIMP_PRINT_H
#include INCLUDE_GIMP_PRINT_H
#else
#include <gimp-print/gimp-print.h>
#endif
#include <gimp-print/gimp-print-intl.h>
#include "../../lib/libprintut.h"
#include "gimp-print-cups.h"
#include <stdio.h>

stp_option_t stp_options[] =
{
  { "Brightness",	"stpBrightness",	N_("Brightness") },
  { "Contrast",		"stpContrast",		N_("Contrast") },
  { "Gamma",		"stpGamma",		N_("Gamma") },
  { "Density",		"stpDensity",		N_("Density") },
  { "Cyan",		"stpCyan",		N_("Cyan") },
  { "Magenta",		"stpMagenta",		N_("Magenta") },
  { "Yellow",		"stpYellow",		N_("Yellow") },
  { "Saturation",	"stpSaturation",	N_("Saturation") },
};

int stp_option_count = sizeof(stp_options) / sizeof (stp_option_t);

/*
 * 'initialize_stp_options()' - Initialize the min/max values for
 *                              each STP numeric option.
 */

void
initialize_stp_options(void)
{
  stp_parameter_t desc;
  int i;
  for (i = 0; i < stp_option_count; i++)
    {
      struct stp_option *opt = &(stp_options[i]);
      stp_describe_parameter(stp_default_settings(), opt->iname, &desc);
      if (desc.p_type != STP_PARAMETER_TYPE_DOUBLE)
	{
	  fprintf(stderr, "Parameter %s isn't right!\n", opt->iname);
	  exit(1);
	}
      opt->low = desc.bounds.dbl.lower * 1000;
      opt->high = desc.bounds.dbl.upper * 1000;
      opt->defval = desc.deflt.dbl * 1000;
      opt->step = 50;
    }
}
