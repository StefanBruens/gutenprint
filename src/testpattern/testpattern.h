/*
 * "$Id: testpattern.h,v 1.9 2003/10/13 02:00:07 rlk Exp $"
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gimp-print/gimp-print.h>

extern stp_vars_t tv;

typedef struct
{
  enum {
    E_PATTERN,
    E_XPATTERN,
    E_IMAGE,
    E_GRID
  } t;
  union {
    struct {
      double mins[32];
      double vals[32];
      double gammas[32];
      double levels[32];
      double lower;
      double upper;
    } p;
    struct {
      int ticks;
    } g;
    struct {
      int x;
      int y;
      int bits;
      const char *data;
    } i;
  } d;
} testpattern_t;

/*
 * At least with flex, this forbids the scanner from reading ahead.
 * This is necessary for parsing images.
 */
#define YY_ALWAYS_INTERACTIVE 1

extern int global_ink_depth;
extern double global_levels[];
extern double global_gammas[];
extern double global_gamma;
extern int steps;
extern double ink_limit;
extern char *printer;
extern double density;
extern double xtop;
extern double xleft;
extern double hsize;
extern double vsize;
extern int noblackline;
extern char *c_strdup(const char *s);
extern testpattern_t *get_next_testpattern(void);

typedef union yylv {
  int ival;
  double dval;
  char *sval;
} YYSTYPE;

#define YYSTYPE_IS_DECLARED 1

#include "testpatterny.h"


