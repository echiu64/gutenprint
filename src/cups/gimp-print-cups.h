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

typedef struct stp_option
{
  const char	*iname,
		*name,			/* Name of option */
    		*text;			/* Human-readable text */
  int		low,			/* Low value (thousandths) */
    		high,			/* High value (thousandths) */
	        defval,			/* Default value */
    		step;			/* Step (thousandths) */
} stp_option_t;

extern stp_option_t stp_options[];
extern int stp_option_count;

extern void initialize_stp_options(void);
