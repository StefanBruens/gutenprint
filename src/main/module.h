/*
 * "$Id: module.h,v 1.1 2003/01/06 20:57:39 rleigh Exp $"
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
#if defined(__APPLE__) || defined(__OpenBSD__)
/* Darwin and OpenBSD prepend underscores to symbols */
#define DLSYM(Handle, Symbol)  stp_dlsym(Handle, "_" Symbol, modulename)
#else
#define DLSYM(Handle, Symbol)  stp_dlsym(Handle, Symbol, modulename)
#endif
#define DLCLOSE(Handle)        dlclose(Handle)
#define DLERROR()              dlerror()
#endif

typedef struct stp_module_version
{
  int major;
  int minor;
} stp_module_version_t;


typedef enum
  {
    STP_MODULE_CLASS_INVALID,
    STP_MODULE_CLASS_MISC,
    STP_MODULE_CLASS_FAMILY,
    STP_MODULE_CLASS_DITHER
  } stp_module_class_t;


typedef struct stp_module
{
  const char *name;         /* module name */
  const char *version;      /* module version number */
  const char *comment;      /* description of module function */
  stp_module_class_t class; /* type of module */
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
} stp_module_t;


int stp_module_load(void);
int stp_module_exit(void);
int stp_module_open(const char *modulename);
int stp_module_init(void);
int stp_module_close(stp_list_item_t *module);
stp_list_t *stp_module_get_class(stp_module_class_t class);


#ifdef __cplusplus
  }
#endif

#endif /* GIMP_PRINT_INTERNAL_MODULE_H */
