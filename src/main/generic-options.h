/*
 * "$Id: generic-options.h,v 1.1 2003/07/19 21:52:25 rlk Exp $"
 *
 *   Copyright 2003 Robert Krawitz (rlk@alum.mit.edu)
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

#ifndef GIMP_PRINT_INTERNAL_GENERIC_OPTIONS_H
#define GIMP_PRINT_INTERNAL_GENERIC_OPTIONS_H

typedef struct
{
  const char *name;
  const char *text;
  int quality_level;		/* Between 0 and 10 */
} stpi_quality_t;

typedef struct
{
  const char *name;
  const char *text;
} stpi_image_type_t;

extern int stpi_get_qualities_count(void);

extern const stpi_quality_t *stpi_get_quality_by_index(int idx);

extern const stpi_quality_t *stpi_get_quality_by_name(const char *quality);

extern int stpi_get_image_types_count(void);

extern const stpi_image_type_t *stpi_get_image_type_by_index(int idx);

extern const stpi_image_type_t *stpi_get_image_type_by_name(const char *image_type);

extern stp_parameter_list_t stpi_list_generic_parameters(stp_const_vars_t v);

extern void stpi_describe_generic_parameter(stp_const_vars_t v,
					   const char *name,
					   stp_parameter_t *description);

#endif /* GIMP_PRINT_INTERNAL_GENERIC_OPTIONS_H */
