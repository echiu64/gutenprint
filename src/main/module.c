/*
 * "$Id$"
 *
 *   libgimpprint module loader - load modules with libltdl/libdl.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <unistd.h>
#include "module.h"
#include "path.h"


typedef struct stpi_internal_module_class
{
  stpi_module_class_t class;
  const char *description;
} stpi_internal_module_class_t;


static void module_list_freefunc(void *item);
static int stpi_module_register(stpi_module_t *module);
#ifdef USE_DLOPEN
static void *stpi_dlsym(void *handle, const char *symbol, const char *modulename);
#endif

static const stpi_internal_module_class_t module_classes[] =
  {
    {STPI_MODULE_CLASS_MISC, N_("Miscellaneous (unclassified)")},
    {STPI_MODULE_CLASS_FAMILY, N_("Family driver")},
    {STPI_MODULE_CLASS_COLOR, N_("Color conversion module")},
    {STPI_MODULE_CLASS_DITHER, N_("Dither algorithm")},
    {STPI_MODULE_CLASS_INVALID, NULL} /* Must be last */
  };

#if !defined(USE_LTDL) && !defined(USE_DLOPEN)
extern stpi_module_t print_canon_LTX_stpi_module_data;
extern stpi_module_t print_escp2_LTX_stpi_module_data;
extern stpi_module_t print_lexmark_LTX_stpi_module_data;
extern stpi_module_t print_pcl_LTX_stpi_module_data;
extern stpi_module_t print_ps_LTX_stpi_module_data;
extern stpi_module_t print_olympus_LTX_stpi_module_data;
extern stpi_module_t print_raw_LTX_stpi_module_data;
extern stpi_module_t color_traditional_LTX_stpi_module_data;

/*
 * A list of modules, for use when the modules are linked statically.
 */
static stpi_module_t *static_modules[] =
  {
    &print_ps_LTX_stpi_module_data,
    &print_canon_LTX_stpi_module_data,
    &print_escp2_LTX_stpi_module_data,
    &print_pcl_LTX_stpi_module_data,
    &print_lexmark_LTX_stpi_module_data,
    &print_olympus_LTX_stpi_module_data,
    &print_raw_LTX_stpi_module_data,
    &color_traditional_LTX_stpi_module_data,
    NULL
  };
#endif

static stpi_list_t *module_list = NULL;


/*
 * Callback for removing a module from stpi_module_list.
 */
static void
module_list_freefunc(void *item /* module to remove */)
{
  stpi_module_t *module = (stpi_module_t *) item;
  if (module && module->exit) /* Call the module exit function */
    module->exit();
#if defined(USE_LTDL) || defined(USE_DLOPEN)
  DLCLOSE(module->handle); /* Close the module if it's not static */
#endif
}


/*
 * Load all available modules.  Return nonzero on failure.
 */
int stpi_module_load(void)
{
  /* initialise libltdl */
#ifdef USE_LTDL
  static int ltdl_is_initialised = 0;        /* Is libltdl initialised? */
#endif
  static int module_list_is_initialised = 0; /* Is the module list initialised? */
#if defined(USE_LTDL) || defined(USE_DLOPEN)
  stpi_list_t *dir_list;                      /* List of directories to scan */
  stpi_list_t *file_list;                     /* List of modules to open */
  stpi_list_item_t *file;                     /* Pointer to current module */
#endif

#ifdef USE_LTDL
  if (!ltdl_is_initialised)
    {
      if (lt_dlinit())
	{
	  stpi_erprintf("Error initialising libltdl: %s\n", DLERROR());
	  return 1;
	}
      ltdl_is_initialised = 1;
    }
  /* set default search paths */
  lt_dladdsearchdir(PKGMODULEDIR);
#endif

  /* initialise module_list */
  if (!module_list_is_initialised)
    {
      if (!(module_list = stpi_list_create()))
	return 1;
      stpi_list_set_freefunc(module_list, module_list_freefunc);
      module_list_is_initialised = 1;
    }

  /* search for available modules */
#if defined (USE_LTDL) || defined (USE_DLOPEN)
  if (!(dir_list = stpi_list_create()))
    return 1;
  stpi_list_set_freefunc(dir_list, stpi_list_node_free_data);
  if (getenv("STP_MODULE_PATH"))
    {
      stpi_path_split(dir_list, getenv("STP_MODULE_PATH"));
    }
  else
    {
#ifdef USE_LTDL
      stpi_path_split(dir_list, getenv("LTDL_LIBRARY_PATH"));
      stpi_path_split(dir_list, lt_dlgetsearchpath());
#else
      stpi_path_split(dir_list, PKGMODULEDIR);
#endif
    }
#ifdef USE_LTDL
  file_list = stpi_path_search(dir_list, ".la");
#else
  file_list = stpi_path_search(dir_list, ".so");
#endif
  stpi_list_destroy(dir_list);

  /* load modules */
  file = stpi_list_get_start(file_list);
  while (file)
    {
      stpi_module_open((const char *) stpi_list_item_get_data(file));
      file = stpi_list_item_next(file);
    }

  stpi_list_destroy(file_list);
#else /* use a static module list */
  {
    int i=0;
    while (static_modules[i])
      {
	stpi_module_register(static_modules[i]);
	i++;
      }
  }
#endif
  return 0;
  }


/*
 * Unload all modules and clean up.
 */
int
stpi_module_exit(void)
{
  /* destroy the module list (modules unloaded by callback) */
  if (module_list)
    stpi_list_destroy(module_list);
  /* shut down libltdl (forces close of any unclosed modules) */
#ifdef USE_LTDL
  return lt_dlexit();
#else
  return 0;
#endif
}


/*
 * Find all modules in a given class.
 */
stpi_list_t *
stpi_module_get_class(stpi_module_class_t class /* Module class */)
{
  stpi_list_t *list;                           /* List to return */
  stpi_list_item_t *ln;                        /* Module to check*/

  list = stpi_list_create(); /* No freefunc, so it can be destroyed
			       without unloading any modules! */
  if (!list)
    return NULL;

  ln = stpi_list_get_start(module_list);
  while (ln)
    {
      /* Add modules of the same class to our list */
      if (((stpi_module_t *) stpi_list_item_get_data(ln))->class == class)
	stpi_list_item_create(list, NULL, stpi_list_item_get_data(ln));
      ln = stpi_list_item_next(ln);
    }
  return list;
}


/*
 * Open a module.
 */
int
stpi_module_open(const char *modulename /* Module filename */)
{
#if defined(USE_LTDL) || defined(USE_DLOPEN)
#ifdef USE_LTDL
  lt_dlhandle module;                  /* Handle for module */
#else
  void *module;                        /* Handle for module */
#endif
  stpi_module_version_t *version;       /* Module version */
  stpi_module_t *data;                  /* Module data */
  stpi_list_item_t *reg_module;         /* Pointer to module list nodes */
  int error = 0;                       /* Error status */

  stpi_deprintf(STPI_DBG_MODULE, "stp-module: open: %s\n", modulename);
  while(1)
    {
      module = DLOPEN(modulename);
      if (!module)
	break;

      /* check version is valid */
      version = (stpi_module_version_t *) DLSYM(module, "stpi_module_version");
      if (!version)
	break;
      if (version->major != 1 && version->minor < 0)
	break;

      data = (void *) DLSYM(module, "stpi_module_data");
      if (!data)
	break;
      data->handle = module; /* store module handle */

      /* check same module isn't already loaded */
      reg_module = stpi_list_get_start(module_list);
      while (reg_module)
	{
	  if (!strcmp(data->name, ((stpi_module_t *)
				   stpi_list_item_get_data(reg_module))->name) &&
	      data->class == ((stpi_module_t *)
			      stpi_list_item_get_data(reg_module))->class)
	    {
	      stpi_deprintf(STPI_DBG_MODULE,
			    "stp-module: reject duplicate: %s\n",
			    data->name);
	      error = 1;
	      break;
	    }
	  reg_module = stpi_list_item_next(reg_module);
	}
      if (error)
	break;

      /* Register the module */
      if (stpi_module_register(data))
	break;

      return 0;
    }

  if (module)
    DLCLOSE(module);
#endif
  return 1;
}


/*
 * Register a loaded module.
 */
static int stpi_module_register(stpi_module_t *module /* Module to register */)
{
  /* Add to the module list */
  if (stpi_list_item_create(module_list, NULL, module))
    return 1;

  stpi_deprintf(STPI_DBG_MODULE, "stp-module: register: %s\n", module->name);
  return 0;
}


/*
 * Initialise all loaded modules
 */
int stpi_module_init(void)
{
  stpi_list_item_t *module_item; /* Module list pointer */
  stpi_module_t *module;         /* Module to initialise */

  module_item = stpi_list_get_start(module_list);
  while (module_item)
    {
      module = (stpi_module_t *) stpi_list_item_get_data(module_item);
      if (module)
	{
	  stpi_deprintf(STPI_DBG_MODULE, "stp-module-init: %s\n", module->name);
	  /* Initialise module */
	  if (module->init && module->init())
	    {
	      stpi_deprintf(STPI_DBG_MODULE,
			    "stp-module-init: %s: Module init failed\n",
			    module->name);
	    }
	}
      module_item = stpi_list_item_next(module_item);
    }
  return 0;
}



/*
 * Close a module.
 */
int
stpi_module_close(stpi_list_item_t *module /* Module to close */)
{
  return stpi_list_item_destroy(module_list, module);
}


/*
 * If using dlopen, add modulename_LTX_ to symbol name
 */
#ifdef USE_DLOPEN
static void *stpi_dlsym(void *handle,           /* Module */
		       const char *symbol,     /* Symbol name */
		       const char *modulename) /* Module name */
{
  int len;                                     /* Length of string */
  static char *full_symbol = NULL;             /* Symbol to return */
  char *module;                                /* Real module name */
  char *tmp = stpi_strdup(modulename);          /* Temporary string */

  module = basename(tmp);

  if (full_symbol)
    {
      stpi_free (full_symbol);
      full_symbol = NULL;
    }

  full_symbol = (char *) stpi_malloc(sizeof(char) * (strlen(module) - 2));

  /* "_LTX_" + '\0' - ".so" */
  len = strlen(symbol) + strlen(module) + 3;
  full_symbol = (char *) stpi_malloc(sizeof(char) * len);

  len = 0;
  strncpy (full_symbol, module, strlen(module) - 3);
  len = strlen(module) - 3;
  strcpy (full_symbol+len, "_LTX_");
  len += 5;
  strcpy (full_symbol+len, symbol);
  len += strlen(symbol);
  full_symbol[len] = '\0';

#if defined(__OpenBSD__)
/* OpenBSD needs a prepended underscore to match symbols */
 {
   char *prefix_symbol = stpi_malloc(sizeof(char) * (strlen(full_symbol) + 2));
   prefix_symbol[0] = '_';
   strcpy(prefix_symbol+1, full_symbol);
   stpi_free(full_symbol);
   full_symbol = prefix_symbol;
 }
#endif

 /* Change any hyphens to underscores */
 for (len = 0; full_symbol[len] != '\0'; len++)
   if (full_symbol[len] == '-')
     full_symbol[len] = '_';

 stpi_deprintf(STPI_DBG_MODULE, "SYMBOL: %s\n", full_symbol);

  return dlsym(handle, full_symbol);
}
#endif
