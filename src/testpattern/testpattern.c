/*
 * "$Id: testpattern.c,v 1.29 2003/10/13 02:00:06 rlk Exp $"
 *
 *   Test pattern generator for Gimp-Print
 *
 *   Copyright 2001 Robert Krawitz <rlk@alum.mit.edu>
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
 * This sample program may be used to generate test patterns.  It also
 * serves as an example of how to use the gimp-print API.
 *
 * As the purpose of this program is to allow fine grained control over
 * the output, it uses the raw CMYK output type.  This feeds 16 bits each
 * of CMYK to the driver.  This mode performs no correction on the data;
 * it passes it directly to the dither engine, performing no color,
 * density, gamma, etc. correction.  Most programs will use one of the
 * other modes (RGB, density and gamma corrected 8-bit CMYK, grayscale, or
 * black and white).
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "testpattern.h"

extern int yyparse(void);

static const char *Image_get_appname(stp_image_t *image);
static void Image_progress_conclude(stp_image_t *image);
static void Image_note_progress(stp_image_t *image,
				double current, double total);
static void Image_progress_init(stp_image_t *image);
static stp_image_status_t Image_get_row(stp_image_t *image,
					unsigned char *data,
					size_t byte_limit, int row);
static int Image_height(stp_image_t *image);
static int Image_width(stp_image_t *image);
static int Image_bpp(stp_image_t *image);
static void Image_init(stp_image_t *image);
static stp_image_t theImage =
{
  Image_init,
  NULL,				/* reset */
  Image_bpp,
  Image_width,
  Image_height,
  Image_get_row,
  Image_get_appname,
  Image_progress_init,
  Image_note_progress,
  Image_progress_conclude,
  NULL
};
stp_vars_t tv;

double global_levels[STP_CHANNEL_LIMIT];
double global_gammas[STP_CHANNEL_LIMIT];
double global_gamma = 1.0;
int steps = 256;
double ink_limit = 1.0;
char *printer = 0;
char *ink_type = 0;
char *resolution = 0;
char *media_source = 0;
char *media_type = 0;
char *media_size = 0;
char *dither_algorithm = 0;
double density = 1.0;
double xtop = 0;
double xleft = 0;
double hsize = 1.0;
double vsize = 1.0;
int noblackline = 0;
int printer_width, printer_height, bandheight;
int n_testpatterns = 0;
int global_ink_depth = 0;

testpattern_t *the_testpatterns = NULL;

static size_t
c_strlen(const char *s)
{
  return strlen(s);
}

static char *
c_strndup(const char *s, int n)
{
  char *ret;
  if (!s || n < 0)
    {
      ret = malloc(1);
      ret[0] = 0;
      return ret;
    }
  else
    {
      ret = malloc(n + 1);
      memcpy(ret, s, n);
      ret[n] = 0;
      return ret;
    }
}

char *
c_strdup(const char *s)
{
  char *ret;
  if (!s)
    {
      ret = malloc(1);
      ret[0] = 0;
      return ret;
    }
  else
    return c_strndup(s, c_strlen(s));
}

testpattern_t *
get_next_testpattern(void)
{
  static int internal_n_testpatterns = 0;
  if (n_testpatterns == 0)
    {
      the_testpatterns = malloc(sizeof(testpattern_t));
      n_testpatterns = internal_n_testpatterns = 1;
      return &(the_testpatterns[0]);
    }
  else if (n_testpatterns >= internal_n_testpatterns)
    {
      internal_n_testpatterns *= 2;
      the_testpatterns =
	realloc(the_testpatterns,
		internal_n_testpatterns * sizeof(testpattern_t));
    }
  return &(the_testpatterns[n_testpatterns++]);
}

static void
do_help(void)
{
  fprintf(stderr, "%s", "\
Usage: testpattern -p printer [-n ramp_steps] [-I ink_limit] [-i ink_type]\n\
                   [-r resolution] [-s media_source] [-t media_type]\n\
                   [-z media_size] [-d dither_algorithm] [-e density]\n\
                   [-G gamma] [-q] [-H width] [-V height] [-T top] [-L left]\n\
       -H, -V, -T, -L expressed as fractions of the printable paper size\n\
       0.0 < ink_limit <= 1.0\n\
       1 < ramp_steps <= 4096\n\
       0.1 <= density <= 2.0\n\
       0.0 < cyan_level <= 10.0 same for magenta and yellow.\n");
  exit(1);
}

static void
writefunc(void *file, const char *buf, size_t bytes)
{
  FILE *prn = (FILE *)file;
  fwrite(buf, 1, bytes, prn);
}

int
main(int argc, char **argv)
{
  int c;
  stp_vars_t v;
  stp_const_printer_t the_printer;
  const stp_papersize_t *pt;
  int left, right, top, bottom;
  int x, y;
  int width, height;
  int retval;
  stp_parameter_list_t params;
  int count;
  int i;

  for (i = 0; i < STP_CHANNEL_LIMIT; i++)
    {
      global_levels[i] = 1.0;
      global_gammas[i] = 1.0;
    }
  stp_init();
  tv = stp_vars_create();
  stp_set_outfunc(tv, writefunc);
  stp_set_errfunc(tv, writefunc);
  stp_set_outdata(tv, stdout);
  stp_set_errdata(tv, stderr);

  retval = yyparse();
  if (retval)
    return retval;

  while (1)
    {
      c = getopt(argc, argv, "qp:n:l:I:r:s:t:z:d:he:T:L:H:V:G:");
      if (c == -1)
	break;
      switch (c)
	{
	case 'I':
	  ink_limit = strtod(optarg, 0);
	  break;
	case 'G':
	  global_gamma = strtod(optarg, 0);
	  break;
	case 'H':
	  hsize = strtod(optarg, 0);
	  break;
	case 'L':
	  xleft = strtod(optarg, 0);
	  break;
	case 'T':
	  xtop = strtod(optarg, 0);
	  break;
	case 'V':
	  vsize = strtod(optarg, 0);
	  break;
	case 'd':
	  stp_set_string_parameter(tv, "DitherAlgorithm", optarg);
	  break;
	case 'e':
	  density = strtod(optarg, 0);
	  break;
	case 'h':
	  do_help();
	  break;
	case 'i':
	  stp_set_string_parameter(tv, "InkType", optarg);
	  break;
	case 'n':
	  steps = atoi(optarg);
	  break;
	case 'p':
	  printer = optarg;
	  break;
	case 'q':
	  noblackline = 1;
	  break;
	case 'r':
	  stp_set_string_parameter(tv, "Resolution", optarg);
	  break;
	case 's':
	  stp_set_string_parameter(tv, "InputSlot", optarg);
	  break;
	case 't':
	  stp_set_string_parameter(tv, "MediaType", optarg);
	  break;
	case 'z':
	  stp_set_string_parameter(tv, "PageSize", optarg);
	  break;
	default:
	  fprintf(stderr, "Unknown option '-%c'\n", c);
	  do_help();
	  break;
	}
    }
  v = stp_vars_create();
  the_printer = stp_get_printer_by_driver(printer);
  if (!printer ||
      ink_limit <= 0 || ink_limit > 1.0 ||
      steps < 1 || steps > 4096 ||
      xtop < 0 || xtop > 1 || xleft < 0 || xleft > 1 ||
      xtop + vsize > 1 || xleft + hsize > 1 ||
      hsize < 0 || hsize > 1 || vsize < 0 || vsize > 1)
    do_help();
  if (!the_printer)
    {
      the_printer = stp_get_printer_by_long_name(printer);
      if (!the_printer)
	{
	  int j;
	  fprintf(stderr, "Unknown printer %s\nValid printers are:\n",printer);
	  for (j = 0; j < stp_printer_model_count(); j++)
	    {
	      the_printer = stp_get_printer_by_index(j);
	      fprintf(stderr, "%-16s%s\n", stp_printer_get_driver(the_printer),
		      stp_printer_get_long_name(the_printer));
	    }
	  return 1;
	}
    }
  stp_set_printer_defaults(v, the_printer);
  stp_set_outfunc(v, writefunc);
  stp_set_errfunc(v, writefunc);
  stp_set_outdata(v, stdout);
  stp_set_errdata(v, stderr);
  stp_set_float_parameter(v, "Density", density);
  stp_set_string_parameter(v, "Quality", "None");
  stp_set_string_parameter(v, "ImageType", "None");

  params = stp_get_parameter_list(v);
  count = stp_parameter_list_count(params);
  for (i = 0; i < count; i++)
    {
      const stp_parameter_t *p = stp_parameter_list_param(params, i);
      const char *val = stp_get_string_parameter(tv, p->name);
      if (p->p_type == STP_PARAMETER_TYPE_STRING_LIST && val && strlen(val) > 0)
	stp_set_string_parameter(v, p->name, val);
    }
  stp_parameter_list_free(params);

  /*
   * Most programs will not use OUTPUT_RAW_CMYK; OUTPUT_COLOR or
   * OUTPUT_GRAYSCALE are more useful for most purposes.
   */
  if (global_ink_depth)
    stp_set_output_type(v, OUTPUT_RAW_PRINTER);
  else
    stp_set_output_type(v, OUTPUT_RAW_CMYK);

  pt = stp_get_papersize_by_name(stp_get_string_parameter(v, "PageSize"));
  if (!pt)
    {
      fprintf(stderr, "Papersize %s unknown\n", media_size);
      return 1;
    }

  stp_get_imageable_area(v, &left, &right, &bottom, &top);
  stp_describe_resolution(v, &x, &y);
  if (x < 0)
    x = 300;
  if (y < 0)
    y = 300;

  width = right - left;
  height = bottom - top;
  top += height * xtop;
  left += width * xleft;
  if (steps > width)
    steps = width;

#if 0
  width = (width / steps) * steps;
  height = (height / n_testpatterns) * n_testpatterns;
#endif
  stp_set_width(v, width * hsize);
  stp_set_height(v, height * vsize);

  printer_width = width * x / 72;
  printer_height = height * y / 72;

  bandheight = printer_height / n_testpatterns;
  stp_set_left(v, left);
  stp_set_top(v, top);

  stp_merge_printvars(v, stp_printer_get_defaults(the_printer));
  if (stp_print(v, &theImage) != 1)
    return 1;
  stp_vars_free(v);
  free(the_testpatterns);
  return 0;
}

static void
fill_black(unsigned short *data, size_t len, size_t scount)
{
  int i;
  if (global_ink_depth)
    {
      for (i = 0; i < (len / scount) * scount; i++)
	{
	  data[0] = ink_limit * 65535;
	  if (global_ink_depth == 3)
	    {
	      data[1] = ink_limit * 65535;
	      data[2] = ink_limit * 65535;
	    }
	  else if (global_ink_depth == 5)
	    {
	      data[2] = ink_limit * 65535;
	      data[4] = ink_limit * 65535;
	    }
	  data += global_ink_depth;
	}
    }
  else
    {
      for (i = 0; i < (len / scount) * scount; i++)
	{
	  data[3] = ink_limit * 65535;
	  data += 4;
	}
    }
}

static void
fill_grid(unsigned short *data, size_t len, size_t scount, testpattern_t *p)
{
  int i;
  int xlen = (len / scount) * scount;
  int depth = global_ink_depth;
  int errdiv = (p->d.g.ticks) / (xlen - 1);
  int errmod = (p->d.g.ticks) % (xlen - 1);
  int errval  = 0;
  int errlast = -1;
  int errline  = 0;
  if (depth == 0)
    depth = 4;
  for (i = 0; i < xlen; i++)
    {
      if (errline != errlast)
	{
	  errlast = errline;
	  data[0] = 65535;
	}
      errval += errmod;
      errline += errdiv;
      if (errval >= xlen - 1)
	{
	  errval -= xlen - 1;
	  errline++;
	}
      data += depth;
    }
}

static void
fill_colors(unsigned short *data, size_t len, size_t scount, testpattern_t *p)
{
  double mins[4];
  double vals[4];
  double gammas[4];
  double levels[4];
  double lower = p->d.p.lower;
  double upper = p->d.p.upper;
  int i;
  int j;
  int pixels;

  vals[0] = p->d.p.vals[0];
  mins[0] = p->d.p.mins[0];

  for (j = 1; j < 4; j++)
    {
      vals[j] = p->d.p.vals[j] == -2 ? global_levels[j] : p->d.p.vals[j];
      mins[j] = p->d.p.mins[j] == -2 ? global_levels[j] : p->d.p.mins[j];
      levels[j] = p->d.p.levels[j] == -2 ? global_levels[j] : p->d.p.levels[j];
    }
  for (j = 0; j < 4; j++)
    {
      gammas[j] = p->d.p.gammas[j] * global_gamma * global_gammas[j];
      vals[j] -= mins[j];
    }

  if (scount > len)
    scount = len;
  pixels = len / scount;
  for (i = 0; i < scount; i++)
    {
      int k;
      double where = (double) i / ((double) scount - 1);
      double cmyv;
      double kv;
      double val = where;
      double xvals[4];
      for (j = 0; j < 4; j++)
	{
	  if (j > 0)
	    xvals[j] = mins[j] + val * vals[j];
	  else
	    xvals[j] = mins[j] + vals[j];
	  xvals[j] = pow(xvals[j], gammas[j]);
	}

      if (where <= lower)
	kv = 0;
      else if (where > upper)
	kv = where;
      else
	kv = (where - lower) * upper / (upper - lower);
      cmyv = vals[0] * (where - kv);
      xvals[0] *= kv;
      for (j = 1; j < 4; j++)
	xvals[j] += cmyv * levels[j];
      for (j = 0; j < 4; j++)
	{
	  if (xvals[j] > 1)
	    xvals[j] = 1;
	  xvals[j] *= ink_limit * 65535;
	}
      for (k = 0; k < pixels; k++)
	{
	  switch (global_ink_depth)
	    {
	    case 0:
	      for (j = 0; j < 4; j++)
		data[j] = xvals[(j + 1) % 4];
	      data += 4;
	      break;
	    case 1:
	      data[0] = xvals[0];
	      break;
	    case 2:
	      data[0] = xvals[0];
	      data[1] = 0;
	      break;
	    case 4:
	      for (j = 0; j < 4; j++)
		data[j] = xvals[j];
	      break;
	    case 6:
	      data[0] = xvals[0];
	      data[1] = xvals[1];
	      data[2] = 0;
	      data[3] = xvals[2];
	      data[4] = 0;
	      data[5] = xvals[3];
	      break;
	    case 7:
	      for (j = 0; j < 4; j++)
		data[j * 2] = xvals[j];
	      for (j = 1; j < 6; j += 2)
		data[j] = 0;
	      break;
	    }
	  data += global_ink_depth;
	}
    }
}

static void
fill_colors_extended(unsigned short *data, size_t len,
		     size_t scount, testpattern_t *p)
{
  double mins[STP_CHANNEL_LIMIT];
  double vals[STP_CHANNEL_LIMIT];
  double gammas[STP_CHANNEL_LIMIT];
  int i;
  int j;
  int k;
  int pixels;
  int channel_limit = global_ink_depth <= 7 ? 7 : global_ink_depth;

  for (j = 0; j < channel_limit; j++)
    {
      mins[j] = p->d.p.mins[j] == -2 ? global_levels[j] : p->d.p.mins[j];
      vals[j] = p->d.p.vals[j] == -2 ? global_levels[j] : p->d.p.vals[j];
      gammas[j] = p->d.p.gammas[j] * global_gamma * global_gammas[j];
      vals[j] -= mins[j];
    }
  if (scount > len)
    scount = len;
  pixels = len / scount;
  for (i = 0; i < scount; i++)
    {
      double where = (double) i / ((double) scount - 1);
      double val = where;
      double xvals[STP_CHANNEL_LIMIT];

      for (j = 0; j < channel_limit; j++)
	{
	  xvals[j] = mins[j] + val * vals[j];
	  xvals[j] = pow(xvals[j], gammas[j]);
	  xvals[j] *= ink_limit * 65535;
	}
      for (k = 0; k < pixels; k++)
	{
	  switch (global_ink_depth)
	    {
	    case 1:
	      data[0] = xvals[0];
	      break;
	    case 2:
	      data[0] = xvals[0];
	      data[1] = xvals[4];
	      break;
	    case 3:
	      data[0] = xvals[1];
	      data[1] = xvals[2];
	      data[2] = xvals[3];
	      break;
	    case 4:
	      data[0] = xvals[0];
	      data[1] = xvals[1];
	      data[2] = xvals[2];
	      data[3] = xvals[3];
	      break;
	    case 5:
	      data[0] = xvals[1];
	      data[1] = xvals[5];
	      data[2] = xvals[2];
	      data[3] = xvals[6];
	      data[4] = xvals[3];
	      break;
	    case 6:
	      data[0] = xvals[0];
	      data[1] = xvals[1];
	      data[2] = xvals[5];
	      data[3] = xvals[2];
	      data[4] = xvals[6];
	      data[5] = xvals[3];
	      break;
	    case 7:
	      data[0] = xvals[0];
	      data[1] = xvals[4];
	      data[2] = xvals[1];
	      data[3] = xvals[5];
	      data[4] = xvals[2];
	      data[5] = xvals[6];
	      data[6] = xvals[3];
	      break;
	    default:
	      for (j = 0; j < global_ink_depth; j++)
		data[j] = xvals[j];
	    }
	  data += global_ink_depth;
	}
    }
}

extern FILE *yyin;

static stp_image_status_t
Image_get_row(stp_image_t *image, unsigned char *data,
	      size_t byte_limit, int row)
{
  int depth = 4;
  if (global_ink_depth)
    depth = global_ink_depth;
  if (the_testpatterns[0].t == E_IMAGE)
    {
      testpattern_t *t = &(the_testpatterns[0]);
      int total_read = fread(data, 1, t->d.i.x * depth * 2, yyin);
      if (total_read != t->d.i.x * depth * 2)
	{
	  fprintf(stderr, "Read failed!\n");
	  return STP_IMAGE_STATUS_ABORT;
	}
    }
  else
    {
      static int previous_band = -1;
      int band = row / bandheight;
      if (previous_band == -2)
	{
	  memset(data, 0, printer_width * depth * sizeof(unsigned short));
	  switch (the_testpatterns[band].t)
	    {
	    case E_PATTERN:
	      fill_colors((unsigned short *)data, printer_width, steps,
			  &(the_testpatterns[band]));
	      break;
	    case E_XPATTERN:
	      fill_colors_extended((unsigned short *)data, printer_width,
				   steps, &(the_testpatterns[band]));
	      break;
	    case E_GRID:
	      fill_grid((unsigned short *)data, printer_width, steps,
			&(the_testpatterns[band]));
	      break;
	    default:
	      break;
	    }
	  previous_band = band;
	}
      else if (row == printer_height - 1)
	{
	  memset(data, 0, printer_width * depth * sizeof(unsigned short));
	  fill_black((unsigned short *)data, printer_width, steps);
	}
      else if (band >= n_testpatterns)
	memset(data, 0, printer_width * depth * sizeof(unsigned short));
      else if (band != previous_band && band >= 0)
	{
	  memset(data, 0, printer_width * depth * sizeof(unsigned short));
	  switch (the_testpatterns[band].t)
	    {
	    case E_PATTERN:
	      fill_colors((unsigned short *)data, printer_width, steps,
			  &(the_testpatterns[band]));
	      break;
	    case E_XPATTERN:
	      fill_colors_extended((unsigned short *)data, printer_width,
				   steps, &(the_testpatterns[band]));
	      break;
	    case E_GRID:
	      fill_grid((unsigned short *)data, printer_width, steps,
			&(the_testpatterns[band]));
	      break;
	    default:
	      break;
	    }
	  previous_band = band;
	}
    }
  return STP_IMAGE_STATUS_OK;
}

static int
Image_bpp(stp_image_t *image)
{
  if (global_ink_depth)
    return global_ink_depth * 2;
  else
    return 8;
}

static int
Image_width(stp_image_t *image)
{
  if (the_testpatterns[0].t == E_IMAGE)
    return the_testpatterns[0].d.i.x;
  else
    return printer_width;
}

static int
Image_height(stp_image_t *image)
{
  if (the_testpatterns[0].t == E_IMAGE)
    return the_testpatterns[0].d.i.y;
  else
    return printer_height;
}

static void
Image_init(stp_image_t *image)
{
 /* dummy function */
}

static void
Image_progress_init(stp_image_t *image)
{
 /* dummy function */
}

/* progress display */
static void
Image_note_progress(stp_image_t *image, double current, double total)
{
  fprintf(stderr, ".");
}

static void
Image_progress_conclude(stp_image_t *image)
{
  fprintf(stderr, "\n");
}

static const char *
Image_get_appname(stp_image_t *image)
{
  return "Test Pattern";
}
