/*
 * "$Id: gimp-print-ui.h,v 1.3 2003/06/29 14:41:54 rleigh Exp $"
 *
 *   Print plug-in for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu). and Steve Miller (smiller@rni.net
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
 *
 *
 * Revision History:
 *
 *   See ChangeLog
 */

#ifndef __GIMP_PRINT_UI_H__
#define __GIMP_PRINT_UI_H__

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __GNUC__
#define inline __inline__
#endif

#include <gtk/gtk.h>

#include <gimp-print/gimp-print.h>

/*
 * All Gimp-specific code is in this file.
 */

typedef enum
{
  ORIENT_AUTO = -1,
  ORIENT_PORTRAIT = 0,
  ORIENT_LANDSCAPE = 1,
  ORIENT_UPSIDEDOWN = 2,
  ORIENT_SEASCAPE = 3
} orient_t;

typedef struct		/**** Printer List ****/
{
  int		active;		/* Do we know about this printer? */
  char		*name;		/* Name of printer */
  char		*output_to;
  float		scaling;      /* Scaling, percent of printable area */
  orient_t	orientation;
  int		unit;	  /* Units for preview area 0=Inch 1=Metric */
  int		auto_size_roll_feed_paper;
  int		invalid_mask;
  stp_vars_t	v;
} stpui_plist_t;

/*
 * Function prototypes
 */
extern void stpui_plist_set_output_to(stpui_plist_t *p, const char *val);
extern void stpui_plist_set_output_to_n(stpui_plist_t *p, const char *val, int n);
extern const char *stpui_plist_get_output_to(const stpui_plist_t *p);
extern void stpui_plist_set_name(stpui_plist_t *p, const char *val);
extern void stpui_plist_set_name_n(stpui_plist_t *p, const char *val, int n);
extern const char *stpui_plist_get_name(const stpui_plist_t *p);
extern void stpui_plist_copy(stpui_plist_t *vd, const stpui_plist_t *vs);
extern int stpui_plist_add(const stpui_plist_t *key, int add_only);
extern void stpui_printer_initialize(stpui_plist_t *printer);
extern const stpui_plist_t *stpui_get_current_printer(void);

extern void stpui_set_printrc_file(const char *name);
extern const char * stpui_get_printrc_file(void);
extern void stpui_printrc_load (void);
extern void stpui_get_system_printers (void);
extern void stpui_printrc_save (void);
extern void stpui_set_image_filename(const char *);
extern const char *stpui_get_image_filename(void);
extern void stpui_set_errfunc(stp_outfunc_t wfunc);
extern stp_outfunc_t stpui_get_errfunc(void);
extern void stpui_set_errdata(void *errdata);
extern void *stpui_get_errdata(void);

extern gint stpui_do_print_dialog (void);

extern gint stpui_compute_orientation(void);
extern void stpui_set_image_dimensions(gint width, gint height);
extern void stpui_set_image_resolution(gdouble xres, gdouble yres);

typedef guchar *(*get_thumbnail_func_t)(void *data, gint *width, gint *height,
					gint *bpp, gint page);
extern void stpui_set_thumbnail_func(get_thumbnail_func_t);
extern get_thumbnail_func_t stpui_get_thumbnail_func(void);
extern void stpui_set_thumbnail_data(void *);
extern void *stpui_get_thumbnail_data(void);

extern int stpui_print(const stpui_plist_t *printer, stp_image_t *im);


#ifdef __cplusplus
  }
#endif

#endif  /* __GIMP_PRINT_UI_H__ */
