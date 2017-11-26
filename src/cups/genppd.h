/*
 *   PPD file generation program for the CUPS drivers.
 *
 *   Copyright 1993-2008 by Mike Sweet and Robert Krawitz.
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
 *
 * Contents:
 *
 *   main()              - Process files on the command-line...
 *   cat_ppd()           - Copy the named PPD to stdout.
 *   generate_ppd()      - Generate a PPD file.
 *   getlangs()          - Get a list of available translations.
 *   help()              - Show detailed help.
 *   is_special_option() - Determine if an option should be grouped.
 *   list_ppds()         - List the available drivers.
 *   print_group_close() - Close a UI group.
 *   print_group_open()  - Open a new UI group.
 *   printlangs()        - Print list of available translations.
 *   printmodels()       - Print a list of available models.
 *   usage()             - Show program usage.
 *   write_ppd()         - Write a PPD file.
 */

#include "i18n.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <libgen.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <cups/raster.h>

extern char	**getlangs(void);


typedef enum
{
  PPD_STANDARD = 0,
  PPD_SIMPLIFIED = 1,
  PPD_NO_COLOR_OPTS = 2
} ppd_type_t;

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

typedef union
{
#ifdef HAVE_LIBZ
  gzFile gzf;
#endif
  FILE *f;
} gpfile;
typedef gpfile *gpFile;

extern const char *ppdext;
extern const char *cups_modeldir;
extern const char *gpext;
extern int cups_ppd_ps_level;
extern int localize_numbers;
extern int use_base_version;

extern int	write_ppd(gpFile fp, const stp_printer_t *p,
		          const char *language, const char *ppd_location,
			  ppd_type_t ppd_type, const char *filename,
			  int compress);
