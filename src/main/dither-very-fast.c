/*
 * "$Id: dither-very-fast.c,v 1.8 2003/05/30 01:23:24 rlk Exp $"
 *
 *   Very fast dither algorithm
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
print_color_very_fast(const stpi_dither_t *d, stpi_dither_channel_t *dc,
		      int val, int x, int y, unsigned char bit, int length)
{
  int j;
  stpi_dither_segment_t *dd = &(dc->ranges[dc->nlevels - 1]);
  unsigned bits = dd->upper->bits;
  if (bits && val >= ditherpoint(d, &(dc->dithermat), x))
    {
      unsigned char *tptr = dc->ptr + d->ptr_offset;

      /*
       * Lay down all of the bits in the pixel.
       */
      set_row_ends(dc, x);
      for (j = 1; j <= bits; j += j, tptr += length)
	{
	  if (j & bits)
	    tptr[0] |= bit;
	}

    }
}

void
stpi_dither_very_fast(stp_vars_t v,
		      int row,
		      const unsigned short *raw,
		      int duplicate_line,
		      int zero_mask)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  int		x,
		length;
  unsigned char	bit;
  int i;

  int xerror, xstep, xmod;

  if ((zero_mask & ((1 << CHANNEL_COUNT(d)) - 1)) ==
      ((1 << CHANNEL_COUNT(d)) - 1))
    return;

  length = (d->dst_width + 7) / 8;

  bit = 128;
  xstep  = CHANNEL_COUNT(d) * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = 0;

  QUANT(6);
  for (x = 0; x != d->dst_width; x ++)
    {
      for (i = 0; i < CHANNEL_COUNT(d); i++)
	{
	  if (CHANNEL(d, i).ptr && raw[i])
	    print_color_very_fast(d, &(CHANNEL(d, i)), raw[i], x, row,
				  bit, length);
	}

      QUANT(11);
      ADVANCE_UNIDIRECTIONAL(d, bit, raw, CHANNEL_COUNT(d), xerror, xstep, xmod);
      QUANT(13);
    }
}
