/*
 * "$Id: generic-options.c,v 1.2 2003/07/22 12:22:54 rlk Exp $"
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include "generic-options.h"
#include <string.h>

static const stpi_quality_t standard_qualities[] =
{
  { "FastEconomy", N_("Fast Economy"), 0 },
  { "Economy",     N_("Economy"),      1 },
  { "Draft",       N_("Draft"),        3 },
  { "Standard",    N_("Standard"),     5 },
  { "High",        N_("High"),         6 },
  { "Photo",       N_("Photo"),        7 },
  { "HighPhoto",   N_("Super Photo"),  8 },
  { "UltraPhoto",  N_("Ultra Photo"),  9 },
  { "Best",        N_("Best"),        10 },
};

static const stpi_image_type_t standard_image_types[] =
{
  { "Text",         N_("Text") },
  { "Graphics",     N_("Graphics") },
  { "TextGraphics", N_("Mixed Text and Graphics") },
  { "Photo",        N_("Photograph") },
  { "LineArt",      N_("Line Art") },
};

static const stp_parameter_t the_parameters[] =
{
  {
    "Quality", N_("Print Quality"), N_("Print Quality"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 0
  },
  {
    "ImageType", N_("Image Type"), N_("Type of image being printed"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_OUTPUT,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 0
  },
};

static const int the_parameter_count =
sizeof(the_parameters) / sizeof(const stp_parameter_t);

int
stpi_get_qualities_count(void)
{
  return sizeof(standard_qualities) / sizeof(stpi_quality_t);
}

const stpi_quality_t *
stpi_get_quality_by_index(int idx)
{
  if (idx < 0 || idx >= stpi_get_qualities_count())
    return NULL;
  else
    return &(standard_qualities[idx]);
}

const stpi_quality_t *
stpi_get_quality_by_name(const char *quality)
{
  int i;
  if (!quality)
    return NULL;
  for (i = 0; i < stpi_get_qualities_count(); i++)
    {
      const stpi_quality_t *qual = stpi_get_quality_by_index(i);
      if (strcmp(quality, qual->name) == 0)
	return qual;
    }
  return NULL;
}

int
stpi_get_image_types_count(void)
{
  return sizeof(standard_image_types) / sizeof(stpi_image_type_t);
}

const stpi_image_type_t *
stpi_get_image_type_by_index(int idx)
{
  if (idx < 0 || idx >= stpi_get_image_types_count())
    return NULL;
  else
    return &(standard_image_types[idx]);
}

const stpi_image_type_t *
stpi_get_image_type_by_name(const char *image_type)
{
  int i;
  if (!image_type)
    return NULL;
  for (i = 0; i < stpi_get_image_types_count(); i++)
    {
      const stpi_image_type_t *itype = stpi_get_image_type_by_index(i);
      if (strcmp(image_type, itype->name) == 0)
	return itype;
    }
  return NULL;
}

stp_parameter_list_t
stpi_list_generic_parameters(stp_const_vars_t v)
{
  stp_parameter_list_t *ret = stp_parameter_list_create();
  int i;
  for (i = 0; i < the_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(the_parameters[i]));
  return ret;
}

void
stpi_describe_generic_parameter(stp_const_vars_t v, const char *name,
				stp_parameter_t *description)
{
  int		i;
  description->p_type = STP_PARAMETER_TYPE_INVALID;
  if (name == NULL)
    return;

  for (i = 0; i < the_parameter_count; i++)
    if (strcmp(name, the_parameters[i].name) == 0)
      {
	stpi_fill_parameter_settings(description, &(the_parameters[i]));
	break;
      }

  description->deflt.str = NULL;

  if (strcmp(name, "Quality") == 0)
    {
#if 0
      description->bounds.str = stp_string_list_create();
      stp_string_list_add_string(description->bounds.str, "None",
				 _("Manual Control"));
      for (i = 0; i < stpi_get_qualities_count(); i++)
	{
	  const stpi_quality_t *qual = stpi_get_quality_by_index(i);
	  stp_string_list_add_string(description->bounds.str, qual->name,
				     qual->text);
	}
      description->deflt.str = "Standard";
#else
      description->bounds.str = NULL;
      description->p_type = STP_PARAMETER_TYPE_INVALID;
#endif
    }
  if (strcmp(name, "ImageType") == 0)
    {
      description->bounds.str = stp_string_list_create();
      stp_string_list_add_string(description->bounds.str, "None",
				 _("Manual Control"));
      for (i = 0; i < stpi_get_image_types_count(); i++)
	{
	  const stpi_image_type_t *itype = stpi_get_image_type_by_index(i);
	  stp_string_list_add_string(description->bounds.str, itype->name,
				     itype->text);
	}
      description->deflt.str = "TextGraphics";
    }
}

