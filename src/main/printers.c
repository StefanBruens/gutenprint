/*
 * "$Id: printers.c,v 1.19 2003/01/05 23:06:33 rlk Exp $"
 *
 *   Print plug-in driver utility functions for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include <math.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <string.h>
#include <stdlib.h>

#define FMIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct stp_internal_printer
{
  int        cookie;		/* Magic number */
  const char *long_name;	/* Long name for UI */
  const char *family;		/* Printer family */
  int        model;		/* Model number */
  int	     color;
  const stp_printfuncs_t *printfuncs;
  stp_vars_t printvars;
} stp_internal_printer_t;


#include "printlist.h"
#include "printers-oldlist.h"


static const char* stp_printer_namefunc(const stp_list_item_t *item);
static const char* stp_printer_long_namefunc(const stp_list_item_t *item);

static stp_list_t *stp_printer_list;


int
stp_init_printer_list(void)
{
  int i, len;
  stp_internal_printer_t *printer;

  len = sizeof(stp_old_printer_list)/sizeof(stp_old_printer_t);

  if(stp_printer_list)
    stp_list_destroy(stp_printer_list);
  stp_printer_list = stp_list_create();
  stp_list_set_freefunc(stp_printer_list, stp_list_node_free_data);
  stp_list_set_namefunc(stp_printer_list, stp_printer_namefunc);
  stp_list_set_long_namefunc(stp_printer_list, stp_printer_long_namefunc);
  /* stp_list_set_sortfunc(stp_printer_sortfunc); */

  for (i=0; i < len; i++)
    {
      const stp_old_printer_t *src = &(stp_old_printer_list[i]);
      stp_vars_t v = stp_allocate_vars();
      const char *item = src->printer_data;
      printer = stp_malloc(sizeof(stp_internal_printer_t));
      printer->cookie = COOKIE_PRINTER;
      printer->long_name = stp_strdup(src->long_name);
      printer->model = src->model;
      printer->printfuncs = src->printfuncs;
      printer->family = stp_strdup(src->family);
      printer->printvars = v;
      stp_set_driver(v, src->short_name);
      stp_set_output_type(v, src->color);
      while (item && *item)
	{
	  const char *dptr = index(item, '=');
	  if (dptr && (dptr - item) > 0)
	    {
	      char *endptr;
	      double val = strtod(dptr + 1, &endptr);
	      if (endptr && endptr > dptr + 1)
		{
		  char *name = stp_strndup(item, dptr - item);
		  stp_set_float_parameter(v, name, val);
		  item = index(endptr, ';');
		  if (item)
		    item++;
		  stp_free(name);
		}
	    }
	  else
	    break;
	}
      stp_list_item_create(stp_printer_list,
			   stp_list_get_end(stp_printer_list),
			   (void *) printer);
    }
  return 0;
}

int
stp_known_printers(void)
{
  return stp_list_get_length(stp_printer_list);
}

const stp_printer_t
stp_get_printer_by_index(int idx)
{
  stp_list_item_t *printer;
  printer = stp_list_get_item_by_index(stp_printer_list, idx);
  if (printer == NULL)
    return NULL;
  return (const stp_printer_t) stp_list_item_get_data(printer);
}

static const char *
stp_printer_namefunc(const stp_list_item_t *item)
{
  stp_printer_t printer = (stp_printer_t) stp_list_item_get_data(item);
  return stp_printer_get_driver(printer);
}

static const char *
stp_printer_long_namefunc(const stp_list_item_t *item)
{
  stp_printer_t printer = (stp_printer_t) stp_list_item_get_data(item);
  return stp_printer_get_long_name(printer);
}

const char *
stp_printer_get_long_name(const stp_printer_t p)
{
  const stp_internal_printer_t *val = (const stp_internal_printer_t *) p;
  return val->long_name;
}

const char *
stp_printer_get_driver(const stp_printer_t p)
{
  return stp_get_driver(stp_printer_get_printvars(p));
}

const char *
stp_printer_get_family(const stp_printer_t p)
{
  const stp_internal_printer_t *val = (const stp_internal_printer_t *) p;
  return val->family;
}

int
stp_printer_get_model(const stp_printer_t p)
{
  const stp_internal_printer_t *val = (const stp_internal_printer_t *) p;
  return val->model;
}

const stp_printfuncs_t *
stp_printer_get_printfuncs(const stp_printer_t p)
{
  const stp_internal_printer_t *val = (const stp_internal_printer_t *) p;
  return val->printfuncs;
}

const stp_vars_t
stp_printer_get_printvars(const stp_printer_t p)
{
  const stp_internal_printer_t *val = (const stp_internal_printer_t *) p;
  return (stp_vars_t) (val->printvars);
}



const stp_printer_t
stp_get_printer_by_long_name(const char *long_name)
{
  stp_list_item_t *printer;
  printer = stp_list_get_item_by_long_name(stp_printer_list, long_name);
  if (!printer)
    return NULL;
  return (const stp_printer_t) stp_list_item_get_data(printer);
}

const stp_printer_t
stp_get_printer_by_driver(const char *driver)
{
  stp_list_item_t *printer;
  printer = stp_list_get_item_by_name(stp_printer_list, driver);
  if (!printer)
    return NULL;
  return (const stp_printer_t) stp_list_item_get_data(printer);
}

int
stp_get_printer_index_by_driver(const char *driver)
{
  /* There should be no need to ever know the index! */
  int idx = 0;
  for (idx = 0; idx < stp_known_printers(); idx++)
    {
      const stp_printer_t val = stp_get_printer_by_index(idx);
      if (!strcmp(stp_printer_get_driver(val), driver))
	return idx;
    }
  return -1;
}

const stp_printer_t
stp_get_printer(const stp_vars_t v)
{
  return stp_get_printer_by_driver(stp_get_driver(v));
}

int
stp_get_model(const stp_vars_t v)
{
  const stp_printer_t p = stp_get_printer(v);
  return stp_printer_get_model(p);
}

stp_parameter_list_t
stp_printer_list_parameters(const stp_vars_t v)
{
  const stp_printfuncs_t *printfuncs =
    stp_printer_get_printfuncs(stp_get_printer(v));
  return (printfuncs->list_parameters)(v);
}

void
stp_printer_describe_parameter(const stp_vars_t v, const char *name,
			       stp_parameter_t *description)
{
  const stp_printfuncs_t *printfuncs =
    stp_printer_get_printfuncs(stp_get_printer(v));
  (printfuncs->parameters)(v, name, description);
}

void
stp_get_media_size(const stp_vars_t v, int *width, int *height)
{
  const stp_printfuncs_t *printfuncs =
    stp_printer_get_printfuncs(stp_get_printer(v));
  (printfuncs->media_size)(v, width, height);
}

void
stp_get_imageable_area(const stp_vars_t v,
		       int *left, int *right, int *bottom, int *top)
{
  const stp_printfuncs_t *printfuncs =
    stp_printer_get_printfuncs(stp_get_printer(v));
  (printfuncs->imageable_area)(v, left, right, bottom, top);
}

void
stp_get_size_limit(const stp_vars_t v, int *max_width, int *max_height,
		   int *min_width, int *min_height)
{
  const stp_printfuncs_t *printfuncs =
    stp_printer_get_printfuncs(stp_get_printer(v));
  (printfuncs->limit)(v, max_width, max_height, min_width,min_height);
}

void
stp_describe_resolution(const stp_vars_t v, int *x, int *y)
{
  const stp_printfuncs_t *printfuncs =
    stp_printer_get_printfuncs(stp_get_printer(v));
  (printfuncs->describe_resolution)(v, x, y);
}

int
stp_verify(const stp_vars_t v)
{
  const stp_printfuncs_t *printfuncs =
    stp_printer_get_printfuncs(stp_get_printer(v));
  return (printfuncs->verify)(v);
}

int
stp_print(const stp_vars_t v, stp_image_t *image)
{
  const stp_printfuncs_t *printfuncs =
    stp_printer_get_printfuncs(stp_get_printer(v));
  return (printfuncs->print)(v, image);
}

int
stp_start_job(const stp_vars_t v, stp_image_t *image)
{
  const stp_printfuncs_t *printfuncs =
    stp_printer_get_printfuncs(stp_get_printer(v));
  if (!stp_get_verified(v))
    return 0;
  if (stp_get_job_mode(v) == STP_JOB_MODE_JOB)
    return 1;
  if (printfuncs->start_job)
    return (printfuncs->start_job)(v, image);
  else
    return 1;
}

int
stp_end_job(const stp_vars_t v, stp_image_t *image)
{
  const stp_printfuncs_t *printfuncs =
    stp_printer_get_printfuncs(stp_get_printer(v));
  if (!stp_get_verified(v))
    return 0;
  if (stp_get_job_mode(v) == STP_JOB_MODE_JOB)
    return 1;
  if (printfuncs->end_job)
    return (printfuncs->end_job)(v, image);
  else
    return 1;
}

static int
verify_string_param(const stp_vars_t v, const char *parameter,
		    stp_parameter_t *desc)
{
  if (desc->is_mandatory || stp_check_string_parameter(v, parameter))
    {
      const char *checkval = stp_get_string_parameter(v, parameter);
      stp_string_list_t vptr = desc->bounds.str;
      size_t count = stp_string_list_count(vptr);
      int answer = 0;
      int i;
      if (checkval == NULL)
	{
	  if (count == 0)
	    return 1;
	  else
	    {
	      stp_eprintf(v, _("Value must be set for %s\n"), parameter);
	      return 0;
	    }
	}
      if (count > 0)
	{
	  for (i = 0; i < count; i++)
	    if (!strcmp(checkval, stp_string_list_param(vptr, i)->name))
	      {
		answer = 1;
		break;
	      }
	  if (!answer)
	    stp_eprintf(v, _("`%s' is not a valid %s\n"), checkval, parameter);
	}
      else if (strlen(checkval) == 0)
	answer = 1;
      else
	stp_eprintf(v, _("`%s' is not a valid %s\n"), checkval, parameter);
      stp_string_list_free(vptr);
      return answer;
    }
  else
    return 1;
}

static int
verify_double_param(const stp_vars_t v, const char *parameter,
		    stp_parameter_t *desc)
{
  if (desc->is_mandatory || stp_check_float_parameter(v, parameter))
    {
      double checkval = stp_get_float_parameter(v, parameter);
      if (checkval < desc->bounds.dbl.lower ||
	  checkval > desc->bounds.dbl.upper)
	{
	  stp_eprintf(v, _("%s must be between %f and %f\n"),
		      parameter, desc->bounds.dbl.lower,
		      desc->bounds.dbl.upper);
	  return 0;
	}
    }
  return 1;
}

static int
verify_int_param(const stp_vars_t v, const char *parameter,
		 stp_parameter_t *desc)
{
  if (desc->is_mandatory || stp_check_int_parameter(v, parameter))
    {
      int checkval = stp_get_int_parameter(v, parameter);
      if (checkval < desc->bounds.integer.lower ||
	  checkval > desc->bounds.integer.upper)
	{
	  stp_eprintf(v, _("%s must be between %d and %d\n"),
		      parameter, desc->bounds.integer.lower,
		      desc->bounds.integer.upper);
	  return 0;
	}
    }
  return 1;
}

static int
verify_curve_param(const stp_vars_t v, const char *parameter,
		    stp_parameter_t *desc)
{
  /* Curves are automatically within range...or are they? */
  return 1;
}

static int
verify_param(const stp_vars_t v, const char *parameter)
{
  stp_parameter_t desc;
  stp_describe_parameter(v, parameter, &desc);
  switch (desc.p_type)
    {
    case STP_PARAMETER_TYPE_STRING_LIST:
      return verify_string_param(v, parameter, &desc);
    case STP_PARAMETER_TYPE_DOUBLE:
      return verify_double_param(v, parameter, &desc);
    case STP_PARAMETER_TYPE_INT:
      return verify_int_param(v, parameter, &desc);
    case STP_PARAMETER_TYPE_CURVE:
      return verify_curve_param(v, parameter, &desc);
    case STP_PARAMETER_TYPE_RAW:
    case STP_PARAMETER_TYPE_FILE:
      return 1;			/* No way to verify this here */
    default:
      stp_eprintf(v, _("Unknown type parameter %s (%d)\n"),
		  parameter, desc.p_type);
      return 0;
    }
}

#define CHECK_INT_RANGE(v, component)					\
do									\
{									\
  const stp_vars_t max = stp_maximum_settings();			\
  const stp_vars_t min = stp_minimum_settings();			\
  if (stp_get_##component((v)) < stp_get_##component(min) ||		\
      stp_get_##component((v)) > stp_get_##component(max))		\
    {									\
      answer = 0;							\
      stp_eprintf(v, _("%s out of range (value %d, min %d, max %d)\n"),	\
		  #component, stp_get_##component(v),			\
		  stp_get_##component(min), stp_get_##component(max));	\
    }									\
} while (0)

typedef struct
{
  char *data;
  size_t bytes;
} errbuf_t;

static void
fill_buffer_writefunc(void *priv, const char *buffer, size_t bytes)
{
  errbuf_t *errbuf = (errbuf_t *) priv;
  if (errbuf->bytes == 0)
    errbuf->data = stp_malloc(bytes + 1);
  else
    errbuf->data = stp_realloc(errbuf->data, errbuf->bytes + bytes + 1);
  memcpy(errbuf->data + errbuf->bytes, buffer, bytes);
  errbuf->bytes += bytes;
  errbuf->data[errbuf->bytes] = '\0';
}

int
stp_verify_printer_params(const stp_vars_t v)
{
  errbuf_t errbuf;
  stp_outfunc_t ofunc = stp_get_errfunc(v);
  void *odata = stp_get_errdata(v);

  stp_parameter_list_t params;
  const stp_printer_t p = stp_get_printer(v);
  int nparams;
  int i;
  int answer = 1;
  int left, top, bottom, right;
  const stp_vars_t printvars = stp_printer_get_printvars(p);
  const char *pagesize = stp_get_string_parameter(v, "PageSize");

  stp_set_errfunc((stp_vars_t) v, fill_buffer_writefunc);
  stp_set_errdata((stp_vars_t) v, &errbuf);

  errbuf.data = NULL;
  errbuf.bytes = 0;

  /*
   * Note that in raw CMYK mode the user is responsible for not sending
   * color output to black & white printers!
   */
  if (stp_get_output_type(printvars) == OUTPUT_GRAY &&
      stp_get_output_type(v) == OUTPUT_COLOR)
    {
      answer = 0;
      stp_eprintf(v, _("Printer does not support color output\n"));
    }
  if (pagesize && strlen(pagesize) > 0)
    {
      answer &= verify_param(v, "PageSize");
    }
  else
    {
      int width, height, min_height, min_width;
      stp_get_size_limit(v, &width, &height, &min_width, &min_height);
      if (stp_get_page_height(v) <= min_height ||
	  stp_get_page_height(v) > height ||
	  stp_get_page_width(v) <= min_width || stp_get_page_width(v) > width)
	{
	  answer = 0;
	  stp_eprintf(v, _("Image size is not valid\n"));
	}
    }

  stp_get_imageable_area(v, &left, &right, &bottom, &top);

  if (stp_get_top(v) < top)
    {
      answer = 0;
      stp_eprintf(v, _("Top margin must not be less than zero\n"));
    }

  if (stp_get_left(v) < left)
    {
      answer = 0;
      stp_eprintf(v, _("Left margin must not be less than zero\n"));
    }

  if (stp_get_height(v) <= 0)
    {
      answer = 0;
      stp_eprintf(v, _("Height must be greater than zero\n"));
    }

  if (stp_get_width(v) <= 0)
    {
      answer = 0;
      stp_eprintf(v, _("Width must be greater than zero\n"));
    }

  if (stp_get_left(v) + stp_get_width(v) > right)
    {
      answer = 0;
      stp_eprintf(v, _("Image is too wide for the page\n"));
    }

  if (stp_get_top(v) + stp_get_height(v) > bottom)
    {
      answer = 0;
      stp_eprintf(v, _("Image is too long for the page\n"));
    }

  CHECK_INT_RANGE(v, image_type);
  CHECK_INT_RANGE(v, output_type);
  CHECK_INT_RANGE(v, input_color_model);
  CHECK_INT_RANGE(v, output_color_model);
  CHECK_INT_RANGE(v, page_number);
  CHECK_INT_RANGE(v, job_mode);

  params = stp_list_parameters(v);
  nparams = stp_parameter_list_count(params);
  for (i = 0; i < nparams; i++)
    {
      const stp_parameter_t *p = stp_parameter_list_param(params, i);
      if (p->p_class != STP_PARAMETER_CLASS_PAGE_SIZE)
	answer &= verify_param(v, p->name);
    }
  stp_parameter_list_destroy(params);
  stp_set_errfunc((stp_vars_t) v, ofunc);
  stp_set_errdata((stp_vars_t) v, odata);
  stp_set_verified(v, answer);
  if (errbuf.bytes > 0)
    {
      stp_eprintf(v, "%s", errbuf.data);
      stp_free(errbuf.data);
    }
  return answer;
}
