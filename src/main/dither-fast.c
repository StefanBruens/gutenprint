/*
 * "$Id: dither-fast.c,v 1.8 2003/05/26 01:03:56 rlk Exp $"
 *
 *   Fast dither algorithm
 *
 *   Copyright 1997-2003 Michael Sweet (mike@easysw.com) and
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
 *
 * Revision History:
 *
 *   See ChangeLog
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include "dither-impl.h"
#include "dither-inlined-functions.h"

static inline void
print_color_fast(const stpi_dither_t *d, stpi_dither_channel_t *dc, int x, int y,
		 unsigned char bit, int length)
{
  int density = dc->o;
  int adjusted = dc->v;
  int xdensity = density;
  dither_matrix_t *dither_matrix = &(dc->dithermat);
  int i;
  int levels = dc->nlevels - 1;
  int j;
  unsigned char *tptr;
  unsigned bits;

  if (density <= 0 || adjusted <= 0)
    return;
  adjusted *= dc->density_adjustment;
  for (i = levels; i >= 0; i--)
    {
      stpi_dither_segment_t *dd = &(dc->ranges[i]);
      unsigned vmatrix;
      unsigned rangepoint;
      unsigned dpoint;
      unsigned range0;
      stpi_ink_defn_t *subc;

      range0 = dd->lower->range;
      if (xdensity <= range0)
	continue;
      dpoint = ditherpoint(d, dither_matrix, x);

      if (dd->is_same_ink)
	subc = dd->upper;
      else
	{
	  rangepoint =
	    ((unsigned) (xdensity - dd->lower->range)) * 65535 / dd->range_span;
	  rangepoint = rangepoint * dd->upper->range / 65536;
	  if (rangepoint >= dpoint)
	    subc = dd->upper;
	  else
	    subc = dd->lower;
	}
      vmatrix = ((subc->value * dpoint) >> 16);

      /*
       * After all that, printing is almost an afterthought.
       * Pick the actual dot size (using a matrix here) and print it.
       */
      if (adjusted > 0 && adjusted >= vmatrix)
	{
	  bits = subc->bits;
	  tptr = dc->ptr + d->ptr_offset;
	  set_row_ends(dc, x);

	  /*
	   * Lay down all of the bits in the pixel.
	   */
	  for (j = 1; j <= bits; j += j, tptr += length)
	    {
	      if (j & bits)
		tptr[0] |= bit;
	    }
	}
      return;
    }
}

void
stpi_dither_fast(stp_vars_t v,
		 int row,
		 const unsigned short *raw,
		 int duplicate_line,
		 int zero_mask)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  int x, length;
  unsigned char	bit;
  int i;

  int dst_width = d->dst_width;
  int xerror, xstep, xmod;
  if ((zero_mask & ((1 << CHANNEL_COUNT(d)) - 1)) ==
      ((1 << CHANNEL_COUNT(d)) - 1))
    return;

  length = (d->dst_width + 7) / 8;

  bit = 128;
  xstep  = CHANNEL_COUNT(d) * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = 0;

  QUANT(14);
  for (x = 0; x != dst_width; x++)
    {
      for (i = 0; i < CHANNEL_COUNT(d); i++)
	{
	  if (CHANNEL(d, i).ptr)
	    {
	      CHANNEL(d, i).v = raw[i];
	      CHANNEL(d, i).o = CHANNEL(d, i).v;
	      print_color_fast(d, &(CHANNEL(d, i)), x, row, bit, length);
	    }
	}
      QUANT(16);
      ADVANCE_UNIDIRECTIONAL(d, bit, raw, CHANNEL_COUNT(d), xerror, xstep, xmod);
      QUANT(17);
    }
}
