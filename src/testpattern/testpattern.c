/*
 * "$Id: testpattern.c,v 1.21 2003/01/11 15:59:28 rlk Exp $"
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

double global_c_level = 1.0;
double global_c_gamma = 1.0;
double global_lc_level = 1.0;
double global_lc_gamma = 1.0;
double global_m_level = 1.0;
double global_m_gamma = 1.0;
double global_lm_level = 1.0;
double global_lm_gamma = 1.0;
double global_y_level = 1.0;
double global_y_gamma = 1.0;
double global_k_level = 1.0;
double global_k_gamma = 1.0;
double global_lk_level = 1.0;
double global_lk_gamma = 1.0;
double global_gamma = 1.0;
int levels = 256;
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
Usage: testpattern -p printer [-n ramp_levels] [-I ink_limit] [-i ink_type]\n\
                   [-r resolution] [-s media_source] [-t media_type]\n\
                   [-z media_size] [-d dither_algorithm] [-e density]\n\
                   [-c cyan_level] [-m magenta_level] [-y yellow_level]\n\
                   [-C cyan_gamma] [-M magenta_gamma] [-Y yellow_gamma]\n\
                   [-K black_gamma] [-G gamma] [-q]\n\
                   [-H width] [-V height] [-T top] [-L left]\n\
       -H, -V, -T, -L expressed as fractions of the printable paper size\n\
       0.0 < ink_limit <= 1.0\n\
       1 < ramp_levels <= 4096\n\
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
  stp_printer_t the_printer;
  stp_papersize_t pt;
  int left, right, top, bottom;
  int x, y;
  int width, height;
  int retval;
  stp_parameter_list_t params;
  int count;
  int i;

  stp_init();
  tv = stp_allocate_vars();
  stp_set_outfunc(tv, writefunc);
  stp_set_errfunc(tv, writefunc);
  stp_set_outdata(tv, stdout);
  stp_set_errdata(tv, stderr);

  retval = yyparse();
  if (retval)
    return retval;

  while (1)
    {
      c = getopt(argc, argv, "qp:n:l:I:r:s:t:z:d:hC:M:Y:K:e:T:L:H:V:c:m:y:G:");
      if (c == -1)
	break;
      switch (c)
	{
	case 'C':
	  global_c_gamma = strtod(optarg, 0);
	  break;
	case 'I':
	  ink_limit = strtod(optarg, 0);
	  break;
	case 'G':
	  global_gamma = strtod(optarg, 0);
	  break;
	case 'H':
	  hsize = strtod(optarg, 0);
	  break;
	case 'K':
	  global_k_gamma = strtod(optarg, 0);
	  break;
	case 'L':
	  xleft = strtod(optarg, 0);
	  break;
	case 'M':
	  global_m_gamma = strtod(optarg, 0);
	  break;
	case 'T':
	  xtop = strtod(optarg, 0);
	  break;
	case 'V':
	  vsize = strtod(optarg, 0);
	  break;
	case 'Y':
	  global_y_gamma = strtod(optarg, 0);
	  break;
	case 'c':
	  global_c_level = strtod(optarg, 0);
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
	case 'm':
	  global_m_level = strtod(optarg, 0);
	  break;
	case 'n':
	  levels = atoi(optarg);
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
	case 'y':
	  global_y_level = strtod(optarg, 0);
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
  v = stp_allocate_vars();
  the_printer = stp_get_printer_by_driver(printer);
  if (!printer ||
      ink_limit <= 0 || ink_limit > 1.0 ||
      levels < 1 || levels > 4096 ||
      global_c_level <= 0 || global_c_level > 10 ||
      global_m_level <= 0 || global_m_level > 10 ||
      global_y_level <= 0 || global_y_level > 10 ||
      xtop < 0 || xtop > 1 || xleft < 0 || xleft > 1 ||
      xtop + vsize > 1 || xleft + hsize > 1 ||
      hsize < 0 || hsize > 1 || vsize < 0 || vsize > 1)
    do_help();
  if (!the_printer)
    {
      the_printer = stp_get_printer_by_long_name(printer);
      if (!the_printer)
	{
	  int i;
	  fprintf(stderr, "Unknown printer %s\nValid printers are:\n",printer);
	  for (i = 0; i < stp_known_printers(); i++)
	    {
	      the_printer = stp_get_printer_by_index(i);
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

  params = stp_list_parameters(v);
  count = stp_parameter_list_count(params);
  for (i = 0; i < count; i++)
    {
      const stp_parameter_t *p = stp_parameter_list_param(params, i);
      const char *val = stp_get_string_parameter(tv, p->name);
      if (p->p_type == STP_PARAMETER_TYPE_STRING_LIST && val && strlen(val) > 0)
	stp_set_string_parameter(v, p->name, val);
    }
  stp_parameter_list_destroy(params);

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
  if (levels > width)
    levels = width;

#if 0
  width = (width / levels) * levels;
  height = (height / n_testpatterns) * n_testpatterns;
#endif
  stp_set_width(v, width * hsize);
  stp_set_height(v, height * vsize);

  printer_width = width * x / 72;
  printer_height = height * y / 72;

  bandheight = printer_height / n_testpatterns;
  stp_set_left(v, left);
  stp_set_top(v, top);

  stp_merge_printvars(v, stp_printer_get_printvars(the_printer));
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
fill_colors(unsigned short *data, size_t len, size_t scount, testpattern_t *p)
{
  double c_min = p->d.p.c_min == -2 ? global_c_level : p->d.p.c_min;
  double m_min = p->d.p.m_min == -2 ? global_m_level : p->d.p.m_min;
  double y_min = p->d.p.y_min == -2 ? global_y_level : p->d.p.y_min;
  double k_min = p->d.p.k_min;
  double c = p->d.p.c == -2 ? global_c_level : p->d.p.c;
  double m = p->d.p.m == -2 ? global_m_level : p->d.p.m;
  double y = p->d.p.y == -2 ? global_y_level : p->d.p.y;
  double c_gamma = p->d.p.c_gamma * global_gamma * global_c_gamma;
  double m_gamma = p->d.p.m_gamma * global_gamma * global_m_gamma;
  double y_gamma = p->d.p.y_gamma * global_gamma * global_y_gamma;
  double k_gamma = p->d.p.k_gamma * global_gamma * global_k_gamma;
  double k = p->d.p.k;
  double c_level = p->d.p.c_level == -2 ? global_c_level : p->d.p.c_level;
  double m_level = p->d.p.m_level == -2 ? global_m_level : p->d.p.m_level;
  double y_level = p->d.p.y_level == -2 ? global_y_level : p->d.p.y_level;
  double lower = p->d.p.lower;
  double upper = p->d.p.upper;
  int i;
  int j;
  int pixels;
  c -= c_min;
  m -= m_min;
  y -= y_min;
  k -= k_min;
  if (scount > len)
    scount = len;
  pixels = len / scount;
  for (i = 0; i < scount; i++)
    {
      double where = (double) i / ((double) scount - 1);
      double cmyv;
      double kv;
      double val = where;
      double cc = c_min + val * c;
      double mm = m_min + val * m;
      double yy = y_min + val * y;
      double kk = k_min + k;
      cc = pow(cc, c_gamma);
      mm = pow(mm, m_gamma);
      yy = pow(yy, y_gamma);
      kk = pow(kk, k_gamma);
      if (where <= lower)
	kv = 0;
      else if (where > upper)
	kv = where;
      else
	kv = (where - lower) * upper / (upper - lower);
      cmyv = k * (where - kv);
      kk *= kv;
      cc += cmyv * c_level;
      mm += cmyv * m_level;
      yy += cmyv * y_level;
      if (cc > 1.0)
	cc = 1.0;
      if (mm > 1.0)
	mm = 1.0;
      if (yy > 1.0)
	yy = 1.0;
      if (kk > 1.0)
	kk = 1.0;
      cc *= ink_limit * 65535;
      mm *= ink_limit * 65535;
      yy *= ink_limit * 65535;
      kk *= ink_limit * 65535;
      for (j = 0; j < pixels; j++)
	{
	  switch (global_ink_depth)
	    {
	    case 0:
	      data[0] = cc;
	      data[1] = mm;
	      data[2] = yy;
	      data[3] = kk;
	      data += 4;
	      break;
	    case 1:
	      data[0] = kk;
	      break;
	    case 2:
	      data[0] = kk;
	      data[1] = 0;
	      break;
	    case 4:
	      data[0] = kk;
	      data[1] = cc;
	      data[2] = mm;
	      data[3] = yy;
	      break;
	    case 6:
	      data[0] = kk;
	      data[1] = cc;
	      data[2] = 0;
	      data[3] = mm;
	      data[4] = 0;
	      data[5] = yy;
	      break;
	    case 7:
	      data[0] = kk;
	      data[1] = 0;
	      data[2] = cc;
	      data[3] = 0;
	      data[4] = mm;
	      data[5] = 0;
	      data[6] = yy;
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
  double c_min = p->d.p.c_min == -2 ? global_c_level : p->d.p.c_min;
  double m_min = p->d.p.m_min == -2 ? global_m_level : p->d.p.m_min;
  double y_min = p->d.p.y_min == -2 ? global_y_level : p->d.p.y_min;
  double k_min = p->d.p.k_min == -2 ? global_k_level : p->d.p.k_min;
  double lc_min = p->d.p.lc_min == -2 ? global_lc_level : p->d.p.lc_min;
  double lm_min = p->d.p.lm_min == -2 ? global_lm_level : p->d.p.lm_min;
  double lk_min = p->d.p.lk_min == -2 ? global_lk_level : p->d.p.lk_min;
  double c = p->d.p.c == -2 ? global_c_level : p->d.p.c;
  double m = p->d.p.m == -2 ? global_m_level : p->d.p.m;
  double y = p->d.p.y == -2 ? global_y_level : p->d.p.y;
  double k = p->d.p.k == -2 ? global_k_level : p->d.p.k;
  double lc = p->d.p.lc == -2 ? global_lc_level : p->d.p.lc;
  double lm = p->d.p.lm == -2 ? global_lm_level : p->d.p.lm;
  double lk = p->d.p.lk == -2 ? global_lk_level : p->d.p.lk;
  double c_gamma = p->d.p.c_gamma * global_gamma * global_c_gamma;
  double m_gamma = p->d.p.m_gamma * global_gamma * global_m_gamma;
  double y_gamma = p->d.p.y_gamma * global_gamma * global_y_gamma;
  double k_gamma = p->d.p.k_gamma * global_gamma * global_k_gamma;
  double lc_gamma = p->d.p.lc_gamma * global_gamma * global_lc_gamma;
  double lm_gamma = p->d.p.lm_gamma * global_gamma * global_lm_gamma;
  double lk_gamma = p->d.p.lk_gamma * global_gamma * global_lk_gamma;
  int i;
  int j;
  int pixels;
  c -= c_min;
  m -= m_min;
  y -= y_min;
  k -= k_min;
  lc -= lc_min;
  lm -= lm_min;
  lk -= lk_min;
  if (scount > len)
    scount = len;
  pixels = len / scount;
  for (i = 0; i < scount; i++)
    {
      double where = (double) i / ((double) scount - 1);
      double val = where;
      double cc = c_min + val * c;
      double mm = m_min + val * m;
      double yy = y_min + val * y;
      double kk = k_min + val * k;
      double lcc = lc_min + val * lc;
      double lmm = lm_min + val * lm;
      double lkk = lk_min + val * lk;
      cc = pow(cc, c_gamma);
      mm = pow(mm, m_gamma);
      yy = pow(yy, y_gamma);
      kk = pow(kk, k_gamma);
      lcc = pow(lcc, lc_gamma);
      lmm = pow(lmm, lm_gamma);
      lkk = pow(lkk, lk_gamma);
      cc *= ink_limit * 65535;
      mm *= ink_limit * 65535;
      yy *= ink_limit * 65535;
      kk *= ink_limit * 65535;
      lcc *= ink_limit * 65535;
      lmm *= ink_limit * 65535;
      lkk *= ink_limit * 65535;
      for (j = 0; j < pixels; j++)
	{
	  switch (global_ink_depth)
	    {
	    case 1:
	      data[0] = kk;
	      break;
	    case 2:
	      data[0] = kk;
	      data[1] = lkk;
	      break;
	    case 3:
	      data[0] = cc;
	      data[1] = mm;
	      data[2] = yy;
	      break;
	    case 4:
	      data[0] = kk;
	      data[1] = cc;
	      data[2] = mm;
	      data[3] = yy;
	      break;
	    case 5:
	      data[0] = cc;
	      data[1] = lcc;
	      data[2] = mm;
	      data[3] = lmm;
	      data[4] = yy;
	      break;
	    case 6:
	      data[0] = kk;
	      data[1] = cc;
	      data[2] = lcc;
	      data[3] = mm;
	      data[4] = lmm;
	      data[5] = yy;
	      break;
	    case 7:
	      data[0] = kk;
	      data[1] = lkk;
	      data[2] = cc;
	      data[3] = lcc;
	      data[4] = mm;
	      data[5] = lmm;
	      data[6] = yy;
	      break;
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
	  return STP_IMAGE_ABORT;
	}
    }
  else
    {
      static int previous_band = -1;
      int band = row / bandheight;
      if (previous_band == -2)
	{
	  memset(data, 0, printer_width * depth * sizeof(unsigned short));
	  if (the_testpatterns[band].t == E_XPATTERN)
	    fill_colors_extended((unsigned short *)data, printer_width,
				 levels, &(the_testpatterns[band]));
	  else
	    fill_colors((unsigned short *)data, printer_width, levels,
			&(the_testpatterns[band]));
	  previous_band = band;
	}
      else if (row == printer_height - 1)
	{
	  memset(data, 0, printer_width * depth * sizeof(unsigned short));
	  fill_black((unsigned short *)data, printer_width, levels);
	}
      else if (band >= n_testpatterns)
	memset(data, 0, printer_width * depth * sizeof(unsigned short));
      else if (band != previous_band && band >= 0)
	{
	  memset(data, 0, printer_width * depth * sizeof(unsigned short));
	  if (noblackline)
	    {
	      if (the_testpatterns[band].t == E_XPATTERN)
		fill_colors_extended((unsigned short *)data, printer_width,
				     levels, &(the_testpatterns[band]));
	      else
		fill_colors((unsigned short *)data, printer_width, levels,
			    &(the_testpatterns[band]));
	      previous_band = band;
	    }
	  else
	    {
	      fill_black((unsigned short *)data, printer_width, levels);
	      previous_band = -2;
	    }
	}
    }
  return STP_IMAGE_OK;
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
