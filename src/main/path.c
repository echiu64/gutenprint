/*
 * "$Id$"
 *
 *   libgimpprint path functions - split and search paths.
 *
 *   Copyright 2002 Roger Leigh (rleigh@debian.org)
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

#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "path.h"

static int stpi_path_check(const struct dirent *module);
static char *stpi_path_merge(const char *path, const char *file);

/* WARNING: This is not thread safe! -- rlk 20030721 */
static const char *path_check_path;   /* Path for scandir() callback */
static const char *path_check_suffix; /* Suffix for scandir() callback */


/*
 * Make a list of all modules in the search path.
 */
stpi_list_t *
stpi_path_search(stpi_list_t *dirlist, /* List of directories to search */
		const char *suffix)  /* Required filename suffix */
{
  stpi_list_t *findlist;              /* List of files to return */
  stpi_list_item_t *diritem;          /* Current directory */
  struct dirent** module_dir;        /* Current directory contents */
  char *module_name;                 /* File name to check */
  int n;                             /* Number of directory entries */

  if (!dirlist)
    return NULL;

  path_check_suffix = suffix;

  findlist = stpi_list_create();
  if (!findlist)
    return NULL;
  stpi_list_set_freefunc(findlist, stpi_list_node_free_data);

  diritem = stpi_list_get_start(dirlist);
  while (diritem)
    {
      path_check_path = (const char *) stpi_list_item_get_data(diritem);
      stpi_deprintf(STPI_DBG_PATH, "stp-path: directory: %s\n",
		    (const char *) stpi_list_item_get_data(diritem));
      n = scandir ((const char *) stpi_list_item_get_data(diritem),
		   &module_dir, stpi_path_check, alphasort);
      if (n >= 0)
	{
	  int idx;
	  for (idx = 0; idx < n; ++idx)
	    {
	      module_name = stpi_path_merge((const char *) stpi_list_item_get_data(diritem),
					   module_dir[idx]->d_name);
	      stpi_list_item_create(findlist, NULL, module_name);
	      stpi_free (module_dir[idx]);
	    }
	  free (module_dir);
	}
      diritem = stpi_list_item_next(diritem);
    }
  return findlist;
}


/*
 * scandir() callback.  Check the filename is sane, has the correct
 * mode bits and suffix.
 */
static int
stpi_path_check(const struct dirent *module) /* File to check */
{
  int namelen;                              /* Filename length */
  int status = 0;                           /* Error status */
  int savederr;                             /* Saved errno */
  char *filename;                           /* Filename */
  struct stat modstat;                      /* stat() output */

  savederr = errno; /* since we are a callback, preserve scandir() state */

  filename = stpi_path_merge(path_check_path, module->d_name);

  namelen = strlen(filename);
  /* make sure we can take off suffix (e.g. .la)
     and still have a sane filename */
  if (namelen >= strlen(path_check_suffix) + 1) 
    {
      if (!stat (filename, &modstat))
	{
	  /* check file exists, and is a regular file */
	  if (S_ISREG(modstat.st_mode))
	    status = 1;
	  if (strncmp(filename + (namelen - strlen(path_check_suffix)),
		      path_check_suffix,
		      strlen(path_check_suffix)))
	    {
	      status = 0;
	    }
	}
    }

  if (status)
    stpi_deprintf(STPI_DBG_PATH, "stp-path: file: `%s'\n", filename);

  stpi_free(filename);
  filename = NULL;

  errno = savederr;
  return status;
}


/*
 * Join a path and filename together.
 */
static char *
stpi_path_merge(const char *path, /* Path */
	       const char *file) /* Filename */
{
  char *filename;                /* Filename to return */
  int namelen = strlen(path) + strlen(file) + 2;
  filename = (char *) stpi_malloc(namelen * sizeof(char));
  strcpy (filename, path);
  strcat (filename, "/");
  strcat (filename, file);
  filename[namelen - 1] = '\0';
  return filename;
}


/*
 * Split a PATH-type string (colon-delimited) into separate
 * directories.
 */
void
stpi_path_split(stpi_list_t *list, /* List to add directories to */
	       const char *path) /* Path to split */
{
  const char *start = path;      /* Start of path name */
  const char *end = NULL;        /* End of path name */
  char *dir = NULL;              /* Path name */
  int len;                       /* Length of path name */

  while (start)
    {
      end = (const char *) strchr(start, ':');
      if (!end)
	len = strlen(start) + 1;
      else
	len = (end - start);

      if (len && !(len == 1 && !end))
	{
	  dir = (char *) stpi_malloc(len + 1);
	  strncpy(dir, start, len);
	  dir[len] = '\0';
	  stpi_list_item_create(list, NULL, dir);
	}
      if (!end)
	{
	  start = NULL;
	  break;
	}
      start = end + 1;
    }
}
