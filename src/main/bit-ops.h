/*
 * "$Id: bit-ops.h,v 1.1 2003/04/20 03:07:40 rlk Exp $"
 *
 *   Softweave calculator for gimp-print.
 *
 *   Copyright 2000 Charles Briscoe-Smith <cpbs@debian.org>
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

#ifndef GIMP_PRINT_INTERNAL_BIT_OPS_H
#define GIMP_PRINT_INTERNAL_BIT_OPS_H

#ifdef __cplusplus
extern "C" {
#endif

extern void	stpi_fold(const unsigned char *line, int single_height,
			  unsigned char *outbuf);

extern void	stpi_split_2(int height, int bits, const unsigned char *in,
			     unsigned char *outhi, unsigned char *outlo);

extern void	stpi_split_4(int height, int bits, const unsigned char *in,
			     unsigned char *out0, unsigned char *out1,
			     unsigned char *out2, unsigned char *out3);

extern void	stpi_unpack_2(int height, int bits, const unsigned char *in,
			      unsigned char *outlo, unsigned char *outhi);

extern void	stpi_unpack_4(int height, int bits, const unsigned char *in,
			      unsigned char *out0, unsigned char *out1,
			      unsigned char *out2, unsigned char *out3);

extern void	stpi_unpack_8(int height, int bits, const unsigned char *in,
			      unsigned char *out0, unsigned char *out1,
			      unsigned char *out2, unsigned char *out3,
			      unsigned char *out4, unsigned char *out5,
			      unsigned char *out6, unsigned char *out7);

#ifdef __cplusplus
  }
#endif

#endif /* GIMP_PRINT_INTERNAL_BIT_OPS_H */
