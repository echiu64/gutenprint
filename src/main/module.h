/*
 * "$Id$"
 *
 *   libgimpprint module loader header
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and Michael Natterer (mitch@gimp.org)
 *   Copyright 2002 Roger Leigh (roger@whinlatter.uklinux.net)
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

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifndef GIMP_PRINT_INTERNAL_MODULE_H
#define GIMP_PRINT_INTERNAL_MODULE_H

#ifdef __cplusplus
extern "C" {
#endif



#ifdef USE_LTDL
#include <ltdl.h>
#elif defined(USE_DLOPEN)
#include <dlfcn.h>
#endif


#ifdef USE_LTDL
#define DLOPEN(Filename)       lt_dlopen(Filename)
#define DLSYM(Handle, Symbol)  lt_dlsym(Handle, Symbol)
#define DLCLOSE(Handle)        lt_dlclose(Handle)
#define DLERROR()              lt_dlerror()
#elif defined(USE_DLOPEN)
#define DLOPEN(Filename)       dlopen(Filename, RTLD_LAZY)
#define DLSYM(Handle, Symbol)  stpi_dlsym(Handle, Symbol, modulename)
#define DLCLOSE(Handle)        dlclose(Handle)
#define DLERROR()              dlerror()
#endif

typedef struct stpi_module_version
{
  int major;
  int minor;
} stpi_module_version_t;


typedef enum
{
  STPI_MODULE_CLASS_INVALID,
  STPI_MODULE_CLASS_MISC,
  STPI_MODULE_CLASS_FAMILY,
  STPI_MODULE_CLASS_DITHER
} stpi_module_class_t;


typedef struct stpi_module
{
  const char *name;         /* module name */
  const char *version;      /* module version number */
  const char *comment;      /* description of module function */
  stpi_module_class_t class; /* type of module */
#ifdef USE_LTDL
  lt_dlhandle handle;       /* ltdl module pointer (set by libgimpprint) */
#else
  void *handle;             /* dlopen or static module pointer */
#endif
  int (*init)(void);        /* initialisation function */
  int (*exit)(void);        /* cleanup and removal function */
  void *syms;               /* pointer to e.g. a struct containing
                               internal module symbols (class-specific
                               functions and data) */
} stpi_module_t;


int stpi_module_load(void);
int stpi_module_exit(void);
int stpi_module_open(const char *modulename);
int stpi_module_init(void);
int stpi_module_close(stpi_list_item_t *module);
stpi_list_t *stpi_module_get_class(stpi_module_class_t class);


#ifdef __cplusplus
  }
#endif

#endif /* GIMP_PRINT_INTERNAL_MODULE_H */
