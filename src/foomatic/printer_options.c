/*
 * "$Id: printer_options.c,v 1.26 2004/03/12 02:06:36 rlk Exp $"
 *
 *   Dump the per-printer options for Grant Taylor's *-omatic database
 *
 *   Copyright 2000 Robert Krawitz (rlk@alum.mit.edu)
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
#include <stdio.h>
#include <string.h>
#ifdef INCLUDE_GIMP_PRINT_H
#include INCLUDE_GIMP_PRINT_H
#else
#include <gimp-print/gimp-print.h>
#endif
#include "../../lib/libprintut.h"

int
main(int argc, char **argv)
{
  int i, j, k;

  stp_init();
  for (i = 0; i < stp_printer_model_count(); i++)
    {
      stp_parameter_list_t params;
      int nparams;
      stp_const_printer_t printer = stp_get_printer_by_index(i);
      const char *driver = stp_printer_get_driver(printer);
      const char *family = stp_printer_get_family(printer);
      stp_vars_t pv = stp_vars_create_copy(stp_printer_get_defaults(printer));
      int tcount = 0;
      size_t count;
      if (strcmp(family, "ps") == 0 || strcmp(family, "raw") == 0)
	continue;
      printf("# Printer model %s, long name `%s'\n", driver,
	     stp_printer_get_long_name(printer));

      params = stp_get_parameter_list(pv);
      nparams = stp_parameter_list_count(params);

      for (k = 0; k < nparams; k++)
	{
	  const stp_parameter_t *p = stp_parameter_list_param(params, k);
	  stp_parameter_t desc;
	  count = 0;
	  stp_describe_parameter(pv, p->name, &desc);
	  if (desc.p_type == STP_PARAMETER_TYPE_STRING_LIST)
	    {
	      count = stp_string_list_count(desc.bounds.str);
	      if (count > 0)
		{
		  printf("$defaults{'%s'}{'%s'} = '%s';\n",
			 driver, p->name, desc.deflt.str);
		  for (j = 0; j < count; j++)
		    {
		      const stp_param_string_t *param =
			stp_string_list_param(desc.bounds.str, j);
		      printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';\n",
			     driver, p->name, param->name, param->text);
		      if (strcmp(p->name, "Resolution") == 0)
			{
			  int x, y;
			  stp_set_string_parameter(pv, "Resolution",
						   param->name);
			  stp_describe_resolution(pv, &x, &y);
			  if (x > 0 && y > 0)
			    {
			      printf("$stpdata{'%s'}{'%s'}{'%s'} = '%d';\n",
				     driver, "x_resolution", param->name, x);
			      printf("$stpdata{'%s'}{'%s'}{'%s'} = '%d';\n",
				     driver, "y_resolution", param->name, y);
			    }
			}
		    }
		}
	    }
	  else if (desc.p_type == STP_PARAMETER_TYPE_DOUBLE)
	    {
	      printf("$stp_float_values{'%s'}{'MINVAL'}{'%s'} = %.3f\n",
		     driver, desc.name, desc.bounds.dbl.lower);
	      printf("$stp_float_values{'%s'}{'MAXVAL'}{'%s'} = %.3f\n",
		     driver, desc.name, desc.bounds.dbl.upper);
	      printf("$stp_float_values{'%s'}{'DEFVAL'}{'%s'} = %.3f\n",
		     driver, desc.name, desc.deflt.dbl);
	    }
	  else if (desc.p_type == STP_PARAMETER_TYPE_INT)
	    {
	      printf("$stp_int_values{'%s'}{'MINVAL'}{'%s'} = %d\n",
		     driver, desc.name, desc.bounds.integer.lower);
	      printf("$stp_int_values{'%s'}{'MAXVAL'}{'%s'} = %d\n",
		     driver, desc.name, desc.bounds.integer.upper);
	      printf("$stp_int_values{'%s'}{'DEFVAL'}{'%s'} = %d\n",
		     driver, desc.name, desc.deflt.integer);
	    }
	  stp_parameter_description_free(&desc);
	  tcount += count;
	}
      stp_parameter_list_free(params);
      if (tcount > 0)
	{
	  if (stp_get_output_type(pv) == OUTPUT_COLOR)
	    {
	      printf("$defaults{'%s'}{'%s'} = '%s';\n",
		     driver, "Color", "Color");
	      printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';\n",
		     driver, "Color", "Color", "Color");
	      printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';\n",
		     driver, "Color", "RawCMYK", "Raw CMYK");
	    }
	  else
	    printf("$defaults{'%s'}{'%s'} = '%s';\n",
		   driver, "Color", "Grayscale");
	  printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';\n",
		 driver, "Color", "Grayscale", "Gray Scale");
	  printf("$stpdata{'%s'}{'%s'}{'%s'} = '%s';\n",
		 driver, "Color", "BlackAndWhite", "Black and White");
	}
      stp_vars_free(pv);
    }
  return 0;
}
