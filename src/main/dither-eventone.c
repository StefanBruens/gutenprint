/*
 * "$Id: dither-eventone.c,v 1.23 2003/11/02 00:29:13 rlk Exp $"
 *
 *   EvenTone dither implementation for Gimp-Print
 *
 *   Copyright 2002-2003 Mark Tomlinson (mark@southern.co.nz)
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
 *   This code uses the Eventone dither algorithm. This is described
 *   at the website http://www.artofcode.com/eventone/
 *   This algorithm is covered by US Patents 5,055,942 and 5,917,614
 *   and was invented by Raph Levien <raph@acm.org>
 *   It was made available to be used free of charge in GPL-licensed
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include <string.h>
#include <math.h>
#include "dither-impl.h"
#include "dither-inlined-functions.h"

typedef struct
{
  int dx;
  int dy;
  int r_sq;
} distance_t;

typedef struct
{
  int	d2x;
  int   d2y;
  distance_t	d_sq;
  int	aspect;
  int	physical_aspect;
  int	diff_factor;
} eventone_t;

typedef struct shade_segment
{
  distance_t dis;
  distance_t *et_dis;
} shade_distance_t;


#define EVEN_C1 256
#define EVEN_C2 (EVEN_C1 * sqrt(3.0) / 2.0)

static void
free_eventone_data(stpi_dither_t *d)
{
  int i;
  for (i = 0; i < CHANNEL_COUNT(d); i++)
    {
      if (CHANNEL(d, i).aux_data)
	{
	  shade_distance_t *shade = (shade_distance_t *) CHANNEL(d,i).aux_data;
	  SAFE_FREE(shade->et_dis);
	  SAFE_FREE(CHANNEL(d, i).aux_data);
	}
    }
  SAFE_FREE(d->aux_data);
}

static void
et_setup(stpi_dither_t *d)
{
  static const int diff_factors[] = {1, 10, 16, 23, 32};
  eventone_t *et = stpi_zalloc(sizeof(eventone_t));
  int xa, ya;
  int i;
  for (i = 0; i < CHANNEL_COUNT(d); i++) {
    int size = 2 * MAX_SPREAD + ((d->dst_width + 7) & ~7);
    CHANNEL(d, i).error_rows = 1;
    CHANNEL(d, i).errs = stpi_zalloc(1 * sizeof(int *));
    CHANNEL(d, i).errs[0] = stpi_zalloc(size * sizeof(int));
  }

  xa = d->x_aspect / d->y_aspect;
  if (xa == 0)
    xa = 1;
  et->d_sq.dx = xa * xa;
  et->d2x = 2 * et->d_sq.dx;

  ya = d->y_aspect / d->x_aspect;
  if (ya == 0)
    ya = 1;
  et->d_sq.dy = ya * ya;
  et->d2y = 2 * et->d_sq.dy;

  et->aspect = EVEN_C2 / (xa * ya);
  et->d_sq.r_sq = 0;

  for (i = 0; i < CHANNEL_COUNT(d); i++) {
    int x;
    shade_distance_t *shade = stpi_zalloc(sizeof(shade_distance_t));
    shade->dis = et->d_sq;
    shade->et_dis = stpi_malloc(sizeof(distance_t) * d->dst_width);
    for (x = 0; x < d->dst_width; x++) {
      shade->et_dis[x] = et->d_sq;
    }
    CHANNEL(d, i).aux_data = shade;
  }
  et->physical_aspect = d->y_aspect / d->x_aspect;
  if (et->physical_aspect >= 4)
    et->physical_aspect = 4;
  else if (et->physical_aspect >= 2)
    et->physical_aspect = 2;
  else et->physical_aspect = 1;

  et->diff_factor = diff_factors[et->physical_aspect];

  d->aux_data = et;
  d->aux_freefunc = free_eventone_data;
}

static int
et_initializer(stpi_dither_t *d, int duplicate_line, int zero_mask)
{
  int i;
  if (!d->aux_data)
    et_setup(d);

  if (!duplicate_line) {
    if ((zero_mask & ((1 << CHANNEL_COUNT(d)) - 1)) !=
	((1 << CHANNEL_COUNT(d)) - 1)) {
      d->last_line_was_empty = 0;
    } else {
      d->last_line_was_empty++;
    }
  } else if (d->last_line_was_empty) {
    d->last_line_was_empty++;
  }

  if (d->last_line_was_empty >= 5) {
    return 0;
  } else if (d->last_line_was_empty == 4) {
    for (i = 0; i < CHANNEL_COUNT(d); i++)
      memset(&CHANNEL(d, i).errs[0][MAX_SPREAD], 0, d->dst_width * sizeof(int));
    return 0;
  }
  for (i = 0; i < CHANNEL_COUNT(d); i++)
    CHANNEL(d, i).v = 0;
  return 1;
}

static inline void
advance_eventone_pre(shade_distance_t *sp, eventone_t *et, int x)
{
  distance_t *etd = &sp->et_dis[x];
  int t = sp->dis.r_sq + sp->dis.dx;
  if (t <= etd->r_sq) { 	/* Do eventone calculations */
    sp->dis.r_sq = t;		/* Nearest pixel same as last one */
    sp->dis.dx += et->d2x;
  } else {
    sp->dis = *etd;		/* Nearest pixel is from a previous line */
  }
}

static inline void
eventone_update(stpi_dither_channel_t *dc, eventone_t *et,
		int x, int direction)
{
  shade_distance_t *sp = (shade_distance_t *) dc->aux_data;
  distance_t *etd = &sp->et_dis[x];
  int t = etd->r_sq + etd->dy;		/* r^2 from dot above */
  int u = sp->dis.r_sq + sp->dis.dy;	/* r^2 from dot on this line */
  if (u < t) {				/* If dot from this line is close */
    t = u;				/* Use it instead */
    etd->dx = sp->dis.dx;
    etd->dy = sp->dis.dy;
  }
  etd->dy += et->d2y;

  if (t > 65535) {			/* Do some hard limiting */
    t = 65535;
  }
  etd->r_sq = t;
}

static inline void
diffuse_error(stpi_dither_channel_t *dc, eventone_t *et, int x, int direction)
{
  /*
   * Tests to date show that the second diffusion pattern works better
   * than the first in most cases.  The previous code is being left here
   * in case it is later determined that the original code works better.
   * -- rlk 20031101
   */
#if 0
/*  int fraction = (dc->v + (et->diff_factor>>1)) / et->diff_factor; */
  int frac_2 = dc->v + dc->v;
  int frac_3 = frac_2 + dc->v;
  dc->errs[0][x + MAX_SPREAD] = frac_3;
  dc->errs[0][x + MAX_SPREAD - direction] += frac_2;
  dc->v -= (frac_2 + frac_3) / 16;
#else
  dc->errs[0][x + MAX_SPREAD] = dc->v * 3;
  dc->errs[0][x + MAX_SPREAD - direction] += dc->v * 5;
  dc->errs[0][x + MAX_SPREAD - (direction * 2)] += dc->v * 1;
  dc->v -= dc->v * 9 / 16;
#endif
}

static inline int
eventone_adjust(stpi_dither_channel_t *dc, eventone_t *et, int dither_point,
		unsigned int desired)
{
  if (dither_point <= 0)
    return 0;
  else if (dither_point >= 65535)
    return 65535;
  if (desired == 0) {
    dither_point = 0;
  } else {
    shade_distance_t *shade = (shade_distance_t *) dc->aux_data;
    dither_point += shade->dis.r_sq * et->aspect - (EVEN_C1 * 65535) / desired;
    if (dither_point > 65535)
      dither_point = 65535;
    else if (dither_point < 0)
      dither_point = 0;
  }
  return dither_point;
}

static inline void
find_segment(stpi_dither_channel_t *dc, unsigned inkval,
	     stpi_ink_defn_t *lower, stpi_ink_defn_t *upper)
{
  lower->range = 0;
  lower->bits = 0;

  if (dc->nlevels == 1)
    {
      upper->bits = dc->ink_list[1].bits;
      upper->range = dc->ink_list[1].value;
    }
  else
    {
      int i;
      stpi_ink_defn_t *ip;

      for (i=0, ip = dc->ink_list; i < dc->nlevels - 1; i++, ip++) {
	if (ip->value > inkval)
	  break;
	lower->bits = ip->bits;
	lower->range = ip->value;
      }

      upper->bits = ip->bits;
      upper->range = ip->value;
    }
}

static inline int
find_segment_and_ditherpoint(stpi_dither_channel_t *dc, unsigned inkval,
			     stpi_ink_defn_t *lower, stpi_ink_defn_t *upper)
{
  find_segment(dc, inkval, lower, upper);
  if (inkval <= lower->range)
    return 0;
  else if (inkval >= upper->range)
    return 65535;
  else
    return (65535u * (inkval - lower->range)) / (upper->range - lower->range);
}

static inline void
print_ink(stpi_dither_t *d, unsigned char *tptr, const stpi_ink_defn_t *ink,
	  unsigned char bit, int length)
{
  int j;

  if (tptr != 0)
    {
      tptr += d->ptr_offset;
      switch(ink->bits)
	{
	case 1:
	  tptr[0] |= bit;
	  return;
	case 2:
	  tptr[length] |= bit;
	  return;
	case 3:
	  tptr[0] |= bit;
	  tptr[length] |= bit;
	  return;
	default:
	  for (j=1; j <= ink->bits; j+=j, tptr += length) {
	    if (j & ink->bits)
	      *tptr |= bit;
	  }
	  return;
	}
    }
}

void
stpi_dither_et(stp_vars_t v,
	       int row,
	       const unsigned short *raw,
	       int duplicate_line,
	       int zero_mask)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  eventone_t *et;

  int		x,
	        length;
  unsigned char	bit;
  int		i;

  int		terminate;
  int		direction;
  int		xerror, xstep, xmod;
  int		channel_count = CHANNEL_COUNT(d);

  if (!et_initializer(d, duplicate_line, zero_mask))
    return;

  et = (eventone_t *) d->aux_data;

  length = (d->dst_width + 7) / 8;

  if (row & 1) {
    direction = 1;
    x = 0;
    terminate = d->dst_width;
    d->ptr_offset = 0;
  } else {
    direction = -1;
    x = d->dst_width - 1;
    terminate = -1;
    d->ptr_offset = length - 1;
    raw += channel_count * (d->src_width - 1);
  }
  bit = 1 << (7 - (x & 7));
  xstep  = channel_count * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = (xmod * x) % d->dst_width;

  for (; x != terminate; x += direction) {

    int point_error = 0;

    for (i=0; i < channel_count; i++) {
      if (CHANNEL(d, i).ptr)
	{
	  int inkspot;
	  int range_point;
	  stpi_dither_channel_t *dc = &CHANNEL(d, i);
	  shade_distance_t *sp = (shade_distance_t *) dc->aux_data;
	  stpi_ink_defn_t *inkp;
	  stpi_ink_defn_t lower, upper;

	  advance_eventone_pre(sp, et, x);

	  /*
	   * Find which are the two candidate dot sizes.
	   * Rather than use the absolute value of the point to compute
	   * the error, we will use the relative value of the point within
	   * the range to find the two candidate dot sizes.
	   */
	  range_point =
	    find_segment_and_ditherpoint(dc, raw[i], &lower, &upper);

	  /* Incorporate error data from previous line */
	  dc->v += 2 * range_point + (dc->errs[0][x + MAX_SPREAD] + 8) / 16;
	  inkspot = dc->v - range_point;

	  point_error += eventone_adjust(dc, et, inkspot, range_point);

	  /* Determine whether to print the larger or smaller dot */

	  inkp = &lower;
	  if (point_error >= 32768) {
	    point_error -= 65535;
	    inkp = &upper;
	    dc->v -= 131070;
	    sp->dis = et->d_sq;
	  }

	  /* Adjust the error to reflect the dot choice */
	  if (inkp->bits) {

	    set_row_ends(dc, x);

	    /* Do the printing */
	    print_ink(d, dc->ptr, inkp, bit, length);
	  }

	  /* Spread the error around to the adjacent dots */
	  eventone_update(dc, et, x, direction);
	  diffuse_error(dc, et, x, direction);
	}
    }
    if (direction == 1)
      ADVANCE_UNIDIRECTIONAL(d, bit, raw, channel_count, xerror, xstep, xmod);
    else
      ADVANCE_REVERSE(d, bit, raw, channel_count, xerror, xstep, xmod);
  }
  if (direction == -1)
    stpi_dither_reverse_row_ends(d);
}
