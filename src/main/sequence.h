/*
 * "$Id: sequence.h,v 1.1 2003/03/19 20:29:38 rleigh Exp $"
 *
 *   libgimpprint sequence functions.
 *
 *   Copyright 2003 Roger Leigh (roger@whinlatter.uklinux.net)
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
 * compile on generic platforms that don't support glib, gimp, gimpprint, etc.
 */

#ifndef GIMP_PRINT_INTERNAL_SEQUENCE_H
#define GIMP_PRINT_INTERNAL_SEQUENCE_H

#ifdef __cplusplus
extern "C" {
#endif


#define COOKIE_SEQUENCE 0xbf6a28d2


typedef struct
{
  int cookie;
  int recompute_range; /* Do we need to recompute the min and max? */
  double blo;          /* Lower bound */
  double bhi;          /* Upper bound */
  double rlo;          /* Lower range limit */
  double rhi;          /* Upper range limit */
  size_t size;         /* Number of points */
  double *data;        /* Array of doubles */
  float *float_data;   /* Data converted to other form */
  long *long_data;
  unsigned long *ulong_data;
  int *int_data;
  unsigned *uint_data;
  short *short_data;
  unsigned short *ushort_data;
} stpi_internal_sequence_t;


#ifdef __cplusplus
  }
#endif

#endif /* GIMP_PRINT_INTERNAL_SEQUENCE_H */
