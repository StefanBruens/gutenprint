/*
 * "$Id: path.c,v 1.4 2003/01/11 01:58:09 rlk Exp $"
 *
 *   libgimpprint path functions - split and search paths.
 *
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

static int stp_path_check(const struct dirent *module);
static char *stp_path_merge(const char *path, const char *file);

static const char *path_check_path;   /* Path for scandir() callback */
static const char *path_check_suffix; /* Suffix for scandir() callback */


/*
 * Make a list of all modules in the search path.
 */
stp_list_t *
stp_path_search(stp_list_t *dirlist, /* List of directories to search */
		const char *suffix)  /* Required filename suffix */
{
  stp_list_t *findlist;              /* List of files to return */
  stp_list_item_t *diritem;          /* Current directory */
  struct dirent** module_dir;        /* Current directory contents */
  char *module_name;                 /* File name to check */
  int n;                             /* Number of directory entries */

  if (!dirlist)
    return NULL;

  path_check_suffix = suffix;

  findlist = stp_list_create();
  if (!findlist)
    return NULL;
  stp_list_set_freefunc(findlist, stp_list_node_free_data);

  diritem = stp_list_get_start(dirlist);
  while (diritem)
    {
      path_check_path = (const char *) stp_list_item_get_data(diritem);
#ifdef DEBUG
      fprintf(stderr, "stp-path: directory: %s\n", (const char *) stp_list_item_get_data(diritem));
#endif
      n = scandir ((const char *) stp_list_item_get_data(diritem),
		   &module_dir, stp_path_check, alphasort);
      if (n >= 0)
	{
	  int idx;
	  for (idx = 0; idx < n; ++idx)
	    {
	      module_name = stp_path_merge((const char *) stp_list_item_get_data(diritem),
					   module_dir[idx]->d_name);
	      stp_list_item_create(findlist, NULL, module_name);
	      stp_free (module_dir[idx]);
	    }
	  free (module_dir);
	}
      diritem = stp_list_item_next(diritem);
    }
  return findlist;
}


/*
 * scandir() callback.  Check the filename is sane, has the correct
 * mode bits and suffix.
 */
static int
stp_path_check(const struct dirent *module) /* File to check */
{
  int namelen;                              /* Filename length */
  int status = 0;                           /* Error status */
  int savederr;                             /* Saved errno */
  char *filename;                           /* Filename */
  struct stat modstat;                      /* stat() output */

  savederr = errno; /* since we are a callback, preserve scandir() state */

  filename = stp_path_merge(path_check_path, module->d_name);

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

#ifdef DEBUG
  if (status)
  fprintf(stderr, "stp-path: file: `%s'\n", filename);
#endif

  stp_free(filename);
  filename = NULL;

  errno = savederr;
  return status;
}


/*
 * Join a path and filename together.
 */
static char *
stp_path_merge(const char *path, /* Path */
	       const char *file) /* Filename */
{
  char *filename;                /* Filename to return */
  int
    namelen,                     /* Filename length */
    nameidx;                     /* Index into filename */

  namelen = strlen(path) + strlen(file) + 2;
  filename = (char *) stp_malloc(namelen * sizeof(char));
  strncpy (filename, path, strlen(path));
  nameidx = strlen(path);
  filename[nameidx++] = '/';
  strcpy ((filename+nameidx), file);

  return filename;
}


/*
 * Split a PATH-type string (colon-delimited) into separate
 * directories.
 */
void
stp_path_split(stp_list_t *list, /* List to add directories to */
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
	  dir = (char *) stp_malloc(len + 1);
	  strncpy(dir, start, len);
	  dir[len] = '\0';
	  stp_list_item_create(list, NULL, dir);
	}
      if (!end)
	{
	  start = NULL;
	  break;
	}
      start = end + 1;
    }
}
