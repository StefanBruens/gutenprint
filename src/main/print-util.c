/*
 * "$Id: print-util.c,v 1.101 2004/04/04 15:15:09 rlk Exp $"
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
#include <limits.h>
#if defined(HAVE_VARARGS_H) && !defined(HAVE_STDARG_H)
#include <varargs.h>
#else
#include <stdarg.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "generic-options.h"
#include "module.h"
#include "xml.h"

#define FMIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct
{
  stp_outfunc_t ofunc;
  void *odata;
  char *data;
  size_t bytes;
} debug_msgbuf_t;

/*
 * We cannot avoid use of the (non-ANSI) vsnprintf here; ANSI does
 * not provide a safe, length-limited sprintf function.
 */

#define STPI_VASPRINTF(result, bytes, format)				\
{									\
  int current_allocation = 64;						\
  result = stpi_malloc(current_allocation);				\
  while (1)								\
    {									\
      va_list args;							\
      va_start(args, format);						\
      bytes = vsnprintf(result, current_allocation, format, args);	\
      va_end(args);							\
      if (bytes >= 0 && bytes < current_allocation)			\
	break;								\
      else								\
	{								\
	  free (result);						\
	  if (bytes < 0)						\
	    current_allocation *= 2;					\
	  else								\
	    current_allocation = bytes + 1;				\
	  result = stpi_malloc(current_allocation);			\
	}								\
    }									\
}

void
stpi_zprintf(stp_const_vars_t v, const char *format, ...)
{
  char *result;
  int bytes;
  STPI_VASPRINTF(result, bytes, format);
  (stp_get_outfunc(v))((void *)(stp_get_outdata(v)), result, bytes);
  stpi_free(result);
}

void
stpi_asprintf(char **strp, const char *format, ...)
{
  char *result;
  int bytes;
  STPI_VASPRINTF(result, bytes, format);
  *strp = result;
}

void
stpi_catprintf(char **strp, const char *format, ...)
{
  char *result1;
  char *result2;
  int bytes;
  STPI_VASPRINTF(result1, bytes, format);
  stpi_asprintf(&result2, "%s%s", *strp, result1);
  stpi_free(result1);
  *strp = result2;
}
  

void
stpi_zfwrite(const char *buf, size_t bytes, size_t nitems, stp_const_vars_t v)
{
  (stp_get_outfunc(v))((void *)(stp_get_outdata(v)), buf, bytes * nitems);
}

void
stpi_putc(int ch, stp_const_vars_t v)
{
  unsigned char a = (unsigned char) ch;
  (stp_get_outfunc(v))((void *)(stp_get_outdata(v)), (char *) &a, 1);
}

#define BYTE(expr, byteno) (((expr) >> (8 * byteno)) & 0xff)

void
stpi_put16_le(unsigned short sh, stp_const_vars_t v)
{
  stpi_putc(BYTE(sh, 0), v);
  stpi_putc(BYTE(sh, 1), v);
}

void
stpi_put16_be(unsigned short sh, stp_const_vars_t v)
{
  stpi_putc(BYTE(sh, 1), v);
  stpi_putc(BYTE(sh, 0), v);
}

void
stpi_put32_le(unsigned int in, stp_const_vars_t v)
{
  stpi_putc(BYTE(in, 0), v);
  stpi_putc(BYTE(in, 1), v);
  stpi_putc(BYTE(in, 2), v);
  stpi_putc(BYTE(in, 3), v);
}

void
stpi_put32_be(unsigned int in, stp_const_vars_t v)
{
  stpi_putc(BYTE(in, 3), v);
  stpi_putc(BYTE(in, 2), v);
  stpi_putc(BYTE(in, 1), v);
  stpi_putc(BYTE(in, 0), v);
}

void
stpi_puts(const char *s, stp_const_vars_t v)
{
  (stp_get_outfunc(v))((void *)(stp_get_outdata(v)), s, strlen(s));
}

void
stpi_send_command(stp_const_vars_t v, const char *command,
		 const char *format, ...)
{
  int i = 0;
  char fchar;
  const char *out_str;
  unsigned short byte_count = 0;
  va_list args;

  if (strlen(format) > 0)
    {
      va_start(args, format);
      for (i = 0; i < strlen(format); i++)
	{
	  switch (format[i])
	    {
	    case 'a':
	    case 'b':
	    case 'B':
	    case 'd':
	    case 'D':
	      break;
	    case 'c':
	      (void) va_arg(args, unsigned int);
	      byte_count += 1;
	      break;
	    case 'h':
	    case 'H':
	      (void) va_arg(args, unsigned int);
	      byte_count += 2;
	      break;
	    case 'l':
	    case 'L':
	      (void) va_arg(args, unsigned int);
	      byte_count += 4;
	      break;
	    case 's':
	      out_str = va_arg(args, const char *);
	      byte_count += strlen(out_str);
	      break;
	    }
	}
      va_end(args);
    }

  stpi_puts(command, v);

  va_start(args, format);
  while ((fchar = format[0]) != '\0')
    {
      switch (fchar)
	{
	case 'a':
	  stpi_putc(byte_count, v);
	  break;
	case 'b':
	  stpi_put16_le(byte_count, v);
	  break;
	case 'B':
	  stpi_put16_be(byte_count, v);
	  break;
	case 'd':
	  stpi_put32_le(byte_count, v);
	  break;
	case 'D':
	  stpi_put32_be(byte_count, v);
	  break;
	case 'c':
	  stpi_putc(va_arg(args, unsigned int), v);
	  break;
	case 'h':
	  stpi_put16_le(va_arg(args, unsigned int), v);
	  break;
	case 'H':
	  stpi_put16_be(va_arg(args, unsigned int), v);
	  break;
	case 'l':
	  stpi_put32_le(va_arg(args, unsigned int), v);
	  break;
	case 'L':
	  stpi_put32_be(va_arg(args, unsigned int), v);
	  break;
	case 's':
	  stpi_puts(va_arg(args, const char *), v);
	  break;
	}
      format++;
    }
  va_end(args);
}

void
stpi_eprintf(stp_const_vars_t v, const char *format, ...)
{
  int bytes;
  if (stp_get_errfunc(v))
    {
      char *result;
      STPI_VASPRINTF(result, bytes, format);
      (stp_get_errfunc(v))((void *)(stp_get_errdata(v)), result, bytes);
      free(result);
    }
  else
    {
      va_list args;
      va_start(args, format);
      vfprintf(stderr, format, args);
      va_end(args);
    }
}

void
stpi_erputc(int ch)
{
  putc(ch, stderr);
}

void
stpi_erprintf(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}

static unsigned long stpi_debug_level = 0;

static void
stpi_init_debug(void)
{
  static int debug_initialized = 0;
  if (!debug_initialized)
    {
      const char *dval = getenv("STP_DEBUG");
      debug_initialized = 1;
      if (dval)
	{
	  stpi_debug_level = strtoul(dval, 0, 0);
	  stpi_erprintf("Gimp-Print %s %s\n", VERSION, RELEASE_DATE);
	}
    }
}

unsigned long
stpi_get_debug_level(void)
{
  stpi_init_debug();
  return stpi_debug_level;
}

void
stpi_dprintf(unsigned long level, stp_const_vars_t v, const char *format, ...)
{
  int bytes;
  stpi_init_debug();
  if ((level & stpi_debug_level) && stp_get_errfunc(v))
    {
      char *result;
      STPI_VASPRINTF(result, bytes, format);
      (stp_get_errfunc(v))((void *)(stp_get_errdata(v)), result, bytes);
      free(result);
    }
}

void
stpi_deprintf(unsigned long level, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  stpi_init_debug();
  if (level & stpi_debug_level)
    vfprintf(stderr, format, args);
  va_end(args);
}

static void
fill_buffer_writefunc(void *priv, const char *buffer, size_t bytes)
{
  debug_msgbuf_t *msgbuf = (debug_msgbuf_t *) priv;
  if (msgbuf->bytes == 0)
    msgbuf->data = stpi_malloc(bytes + 1);
  else
    msgbuf->data = stpi_realloc(msgbuf->data, msgbuf->bytes + bytes + 1);
  memcpy(msgbuf->data + msgbuf->bytes, buffer, bytes);
  msgbuf->bytes += bytes;
  msgbuf->data[msgbuf->bytes] = '\0';
}

void
stpi_init_debug_messages(stp_vars_t v)
{
  int verified_flag = stpi_get_verified(v);
  debug_msgbuf_t *msgbuf = stpi_malloc(sizeof(debug_msgbuf_t));
  msgbuf->ofunc = stp_get_errfunc(v);
  msgbuf->odata = stp_get_errdata(v);
  msgbuf->data = NULL;
  msgbuf->bytes = 0;
  stp_set_errfunc((stp_vars_t) v, fill_buffer_writefunc);
  stp_set_errdata((stp_vars_t) v, msgbuf);
  stpi_set_verified((stp_vars_t) v, verified_flag);
}

void
stpi_flush_debug_messages(stp_vars_t v)
{
  int verified_flag = stpi_get_verified(v);
  debug_msgbuf_t *msgbuf = (debug_msgbuf_t *)stp_get_errdata(v);
  stp_set_errfunc((stp_vars_t) v, msgbuf->ofunc);
  stp_set_errdata((stp_vars_t) v, msgbuf->odata);
  stpi_set_verified((stp_vars_t) v, verified_flag);
  if (msgbuf->bytes > 0)
    {
      stpi_eprintf(v, "%s", msgbuf->data);
      stpi_free(msgbuf->data);
    }
  stpi_free(msgbuf);
}

/* pointers to the allocation functions to use, which may be set by
   client applications */
void *(*stpi_malloc_func)(size_t size) = malloc;
void *(*stpi_realloc_func)(void *ptr, size_t size) = realloc;
void (*stpi_free_func)(void *ptr) = free;

void *
stpi_malloc (size_t size)
{
  register void *memptr = NULL;

  if ((memptr = stpi_malloc_func (size)) == NULL)
    {
      fputs("Virtual memory exhausted.\n", stderr);
      stpi_abort();
    }
  return (memptr);
}

void *
stpi_zalloc (size_t size)
{
  register void *memptr = stpi_malloc(size);
  (void) memset(memptr, 0, size);
  return (memptr);
}

void *
stpi_realloc (void *ptr, size_t size)
{
  register void *memptr = NULL;

  if (size > 0 && ((memptr = stpi_realloc_func (ptr, size)) == NULL))
    {
      fputs("Virtual memory exhausted.\n", stderr);
      stpi_abort();
    }
  return (memptr);
}

void
stpi_free(void *ptr)
{
  stpi_free_func(ptr);
}

int
stp_init(void)
{
  static int stpi_is_initialised = 0;
  if (!stpi_is_initialised)
    {
      /* Things that are only initialised once */
      /* Set up gettext */
#ifdef ENABLE_NLS
      setlocale (LC_ALL, "");
      bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
#endif
      stpi_init_debug();
      stpi_xml_preinit();
      stpi_init_printer();
      stpi_init_paper();
      stpi_init_dither();
      /* Load modules */
      if (stpi_module_load())
	return 1;
      /* Load XML data */
      if (stpi_xml_init_defaults())
	return 1;
      /* Initialise modules */
      if (stpi_module_init())
	return 1;
      /* Set up defaults for core parameters */
      stpi_initialize_printer_defaults();
    }

  stpi_is_initialised = 1;
  return 0;
}

size_t
stpi_strlen(const char *s)
{
  return strlen(s);
}

char *
stpi_strndup(const char *s, int n)
{
  char *ret;
  if (!s || n < 0)
    {
      ret = stpi_malloc(1);
      ret[0] = 0;
      return ret;
    }
  else
    {
      ret = stpi_malloc(n + 1);
      memcpy(ret, s, n);
      ret[n] = 0;
      return ret;
    }
}

char *
stpi_strdup(const char *s)
{
  char *ret;
  if (!s)
    {
      ret = stpi_malloc(1);
      ret[0] = '\0';
      return ret;
    }
  else
    return stpi_strndup(s, stpi_strlen(s));
}

const char *
stp_set_output_codeset(const char *codeset)
{
#ifdef ENABLE_NLS
  return (const char *)(bind_textdomain_codeset(PACKAGE, codeset));
#else
  return "US-ASCII";
#endif
}

stp_curve_t
stpi_read_and_compose_curves(const char *s1, const char *s2,
			    stp_curve_compose_t comp)
{
  stp_curve_t ret = NULL;
  stp_curve_t t1 = NULL;
  stp_curve_t t2 = NULL;
  if (s1)
    t1 = stp_curve_create_from_string(s1);
  if (s2)
    t2 = stp_curve_create_from_string(s2);
  if (t1 && t2)
    stp_curve_compose(&ret, t1, t2, comp, -1);
  if (ret)
    {
      stp_curve_free(t1);
      stp_curve_free(t2);
      return ret;
    }
  else if (t1)
    {
      stp_curve_free(t2);
      return t1;
    }
  else
    return t2;
}

void
stp_merge_printvars(stp_vars_t user, stp_const_vars_t print)
{
  int i;
  stp_parameter_list_t params = stp_get_parameter_list(print);
  int count = stp_parameter_list_count(params);
  for (i = 0; i < count; i++)
    {
      const stp_parameter_t *p = stp_parameter_list_param(params, i);
      if (p->p_type == STP_PARAMETER_TYPE_DOUBLE &&
	  p->p_class == STP_PARAMETER_CLASS_OUTPUT &&
	  stp_check_float_parameter(print, p->name, STP_PARAMETER_DEFAULTED))
	{
	  stp_parameter_t desc;
	  double prnval = stp_get_float_parameter(print, p->name);
	  double usrval;
	  stp_describe_parameter(print, p->name, &desc);
	  if (stp_check_float_parameter(user, p->name, STP_PARAMETER_ACTIVE))
	    usrval = stp_get_float_parameter(user, p->name);
	  else
	    usrval = desc.deflt.dbl;
	  if (strcmp(p->name, "Gamma") == 0)
	    usrval /= prnval;
	  else
	    usrval *= prnval;
	  if (usrval < desc.bounds.dbl.lower)
	    usrval = desc.bounds.dbl.lower;
	  else if (usrval > desc.bounds.dbl.upper)
	    usrval = desc.bounds.dbl.upper;
	  stp_set_float_parameter(user, p->name, usrval);
	  stp_parameter_description_free(&desc);
	}
    }
  stp_parameter_list_free(params);
}

stp_parameter_list_t
stp_get_parameter_list(stp_const_vars_t v)
{
  stp_parameter_list_t ret = stp_parameter_list_create();
  stp_parameter_list_t tmp_list;

  tmp_list = stpi_printer_list_parameters(v);
  stp_parameter_list_append(ret, tmp_list);
  stp_parameter_list_free(tmp_list);

  tmp_list = stpi_color_list_parameters(v);
  stp_parameter_list_append(ret, tmp_list);
  stp_parameter_list_free(tmp_list);

  tmp_list = stpi_dither_list_parameters(v);
  stp_parameter_list_append(ret, tmp_list);
  stp_parameter_list_free(tmp_list);

  tmp_list = stpi_list_generic_parameters(v);
  stp_parameter_list_append(ret, tmp_list);
  stp_parameter_list_free(tmp_list);

  return ret;
}

void
stpi_abort(void)
{
  abort();
}
