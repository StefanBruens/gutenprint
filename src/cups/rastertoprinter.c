/*
 * "$Id: rastertoprinter.c,v 1.47 2003/01/20 02:08:37 rlk Exp $"
 *
 *   GIMP-print based raster filter for the Common UNIX Printing System.
 *
 *   Copyright 1993-2001 by Easy Software Products.
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License,
 *   version 2, as published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, please contact Easy Software
 *   Products at:
 *
 *       Attn: CUPS Licensing Information
 *       Easy Software Products
 *       44141 Airport View Drive, Suite 204
 *       Hollywood, Maryland 20636-3111 USA
 *
 *       Voice: (301) 373-9603
 *       EMail: cups-info@cups.org
 *         WWW: http://www.cups.org
 *
 * Contents:
 *
 *   main()                    - Main entry and processing of driver.
 *   cups_writefunc()          - Write data to a file...
 *   cancel_job()              - Cancel the current job...
 *   Image_bpp()               - Return the bytes-per-pixel of an image.
 *   Image_get_appname()       - Get the application we are running.
 *   Image_get_row()           - Get one row of the image.
 *   Image_height()            - Return the height of an image.
 *   Image_init()              - Initialize an image.
 *   Image_note_progress()     - Notify the user of our progress.
 *   Image_progress_conclude() - Close the progress display.
 *   Image_progress_init()     - Initialize progress display.
 *   Image_width()             - Return the width of an image.
 */

/*
 * Include necessary headers...
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <cups/cups.h>
#include <cups/ppd.h>
#include <cups/raster.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifdef INCLUDE_GIMP_PRINT_H
#include INCLUDE_GIMP_PRINT_H
#else
#include <gimp-print/gimp-print.h>
#endif
#include "../../lib/libprintut.h"
#include "gimp-print-cups.h"

/*
 * Structure for page raster data...
 */

typedef struct
{
  cups_raster_t		*ras;		/* Raster stream to read from */
  int			page;		/* Current page number */
  int			row;		/* Current row number */
  int			left;
  int			right;
  int			bottom;
  int			top;
  int			width;
  int			height;
  cups_page_header_t	header;		/* Page header from file */
} cups_image_t;

static void	cups_writefunc(void *file, const char *buf, size_t bytes);
static void	cancel_job(int sig);
static const char *Image_get_appname(stp_image_t *image);
static void	 Image_progress_conclude(stp_image_t *image);
static void	Image_note_progress(stp_image_t *image,
				    double current, double total);
static void	Image_progress_init(stp_image_t *image);
static stp_image_status_t Image_get_row(stp_image_t *image,
					unsigned char *data,
					size_t byte_limit, int row);
static int	Image_height(stp_image_t *image);
static int	Image_width(stp_image_t *image);
static int	Image_bpp(stp_image_t *image);
static void	Image_init(stp_image_t *image);

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

static volatile stp_image_status_t Image_status;

static void
set_special_parameter(stp_vars_t v, const char *name, int choice)
{
  stp_parameter_t desc;
  stp_describe_parameter(v, name, &desc);
  if (desc.p_type == STP_PARAMETER_TYPE_STRING_LIST)
    {
      if (choice >= stp_string_list_count(desc.bounds.str))
	fprintf(stderr, "ERROR: Unable to set %s!\n", name);
      else
	stp_set_string_parameter
	  (v, name, stp_string_list_param(desc.bounds.str, choice)->name);
    }
  stp_free_parameter_description(&desc);
}

/*
 * 'main()' - Main entry and processing of driver.
 */

int					/* O - Exit status */
main(int  argc,				/* I - Number of command-line arguments */
     char *argv[])			/* I - Command-line arguments */
{
  int			fd;		/* File descriptor */
  cups_image_t		cups;		/* CUPS image */
  const char		*ppdfile;	/* PPD environment variable */
  ppd_file_t		*ppd;		/* PPD file */
  ppd_option_t		*option;	/* PPD option */
  stp_printer_t		printer;	/* Printer driver */
  stp_vars_t		v;		/* Printer driver variables */
  stp_papersize_t	size;		/* Paper size */
  char			*buffer;	/* Overflow buffer */
  int			num_options;	/* Number of CUPS options */
  cups_option_t		*options;	/* CUPS options */
  const char		*val;		/* CUPS option value */
  int			i;

 /*
  * Initialise libgimpprint
  */

  theImage.rep = &cups;

  stp_init();

 /*
  * Check for valid arguments...
  */

  if (argc < 6 || argc > 7)
  {
   /*
    * We don't have the correct number of arguments; write an error message
    * and return.
    */

    fputs("ERROR: rastertoprinter job-id user title copies options [file]\n", stderr);
    return (1);
  }

  Image_status = STP_IMAGE_OK;

 /*
  * Get the PPD file...
  */

  if ((ppdfile = getenv("PPD")) == NULL)
  {
    fputs("ERROR: Fatal error: PPD environment variable not set!\n", stderr);
    return (1);
  }

  if ((ppd = ppdOpenFile(ppdfile)) == NULL)
  {
    fprintf(stderr, "ERROR: Fatal error: Unable to load PPD file \"%s\"!\n",
            ppdfile);
    return (1);
  }

  if (ppd->modelname == NULL)
  {
    fprintf(stderr, "ERROR: Fatal error: No ModelName attribute in PPD file \"%s\"!\n",
            ppdfile);
    ppdClose(ppd);
    return (1);
  }

 /*
  * Get the STP options, if any...
  */

  initialize_stp_options();

  num_options = cupsParseOptions(argv[5], 0, &options);
  for (i = 0; i < stp_option_count; i++)
    {
      struct stp_option *opt = &(stp_options[i]);
      if ((val = cupsGetOption(opt->name, num_options, options)) != NULL)
	opt->defval = atof(val);
      else if ((option = ppdFindOption(ppd, opt->name)) != NULL)
	opt->defval = atof(option->defchoice);
      opt++;
    }

 /*
  * Figure out which driver to use...
  */

  if ((printer = stp_get_printer_by_driver(ppd->modelname)) == NULL)
  {
    fprintf(stderr, "ERROR: Fatal error: Unable to find driver named \"%s\"!\n",
            ppd->modelname);
    ppdClose(ppd);
    return (1);
  }

  ppdClose(ppd);

 /*
  * Open the page stream...
  */

  if (argc == 7)
  {
    if ((fd = open(argv[6], O_RDONLY)) == -1)
    {
      perror("ERROR: Unable to open raster file - ");
      sleep(1);
      return (1);
    }
  }
  else
    fd = 0;

  cups.ras = cupsRasterOpen(fd, CUPS_RASTER_READ);

 /*
  * Process pages as needed...
  */

  cups.page = 0;

  while (cupsRasterReadHeader(cups.ras, &cups.header))
  {
   /*
    * Update the current page...
    */

    cups.row = 0;

    fprintf(stderr, "PAGE: %d 1\n", cups.page);

   /*
    * Debugging info...
    */

    fprintf(stderr, "DEBUG: StartPage...\n");
    fprintf(stderr, "DEBUG: MediaClass = \"%s\"\n", cups.header.MediaClass);
    fprintf(stderr, "DEBUG: MediaColor = \"%s\"\n", cups.header.MediaColor);
    fprintf(stderr, "DEBUG: MediaType = \"%s\"\n", cups.header.MediaType);
    fprintf(stderr, "DEBUG: OutputType = \"%s\"\n", cups.header.OutputType);

    fprintf(stderr, "DEBUG: AdvanceDistance = %d\n", cups.header.AdvanceDistance);
    fprintf(stderr, "DEBUG: AdvanceMedia = %d\n", cups.header.AdvanceMedia);
    fprintf(stderr, "DEBUG: Collate = %d\n", cups.header.Collate);
    fprintf(stderr, "DEBUG: CutMedia = %d\n", cups.header.CutMedia);
    fprintf(stderr, "DEBUG: Duplex = %d\n", cups.header.Duplex);
    fprintf(stderr, "DEBUG: HWResolution = [ %d %d ]\n", cups.header.HWResolution[0],
            cups.header.HWResolution[1]);
    fprintf(stderr, "DEBUG: ImagingBoundingBox = [ %d %d %d %d ]\n",
            cups.header.ImagingBoundingBox[0], cups.header.ImagingBoundingBox[1],
            cups.header.ImagingBoundingBox[2], cups.header.ImagingBoundingBox[3]);
    fprintf(stderr, "DEBUG: InsertSheet = %d\n", cups.header.InsertSheet);
    fprintf(stderr, "DEBUG: Jog = %d\n", cups.header.Jog);
    fprintf(stderr, "DEBUG: LeadingEdge = %d\n", cups.header.LeadingEdge);
    fprintf(stderr, "DEBUG: Margins = [ %d %d ]\n", cups.header.Margins[0],
            cups.header.Margins[1]);
    fprintf(stderr, "DEBUG: ManualFeed = %d\n", cups.header.ManualFeed);
    fprintf(stderr, "DEBUG: MediaPosition = %d\n", cups.header.MediaPosition);
    fprintf(stderr, "DEBUG: MediaWeight = %d\n", cups.header.MediaWeight);
    fprintf(stderr, "DEBUG: MirrorPrint = %d\n", cups.header.MirrorPrint);
    fprintf(stderr, "DEBUG: NegativePrint = %d\n", cups.header.NegativePrint);
    fprintf(stderr, "DEBUG: NumCopies = %d\n", cups.header.NumCopies);
    fprintf(stderr, "DEBUG: Orientation = %d\n", cups.header.Orientation);
    fprintf(stderr, "DEBUG: OutputFaceUp = %d\n", cups.header.OutputFaceUp);
    fprintf(stderr, "DEBUG: PageSize = [ %d %d ]\n", cups.header.PageSize[0],
            cups.header.PageSize[1]);
    fprintf(stderr, "DEBUG: Separations = %d\n", cups.header.Separations);
    fprintf(stderr, "DEBUG: TraySwitch = %d\n", cups.header.TraySwitch);
    fprintf(stderr, "DEBUG: Tumble = %d\n", cups.header.Tumble);
    fprintf(stderr, "DEBUG: cupsWidth = %d\n", cups.header.cupsWidth);
    fprintf(stderr, "DEBUG: cupsHeight = %d\n", cups.header.cupsHeight);
    fprintf(stderr, "DEBUG: cupsMediaType = %d\n", cups.header.cupsMediaType);
    fprintf(stderr, "DEBUG: cupsBitsPerColor = %d\n", cups.header.cupsBitsPerColor);
    fprintf(stderr, "DEBUG: cupsBitsPerPixel = %d\n", cups.header.cupsBitsPerPixel);
    fprintf(stderr, "DEBUG: cupsBytesPerLine = %d\n", cups.header.cupsBytesPerLine);
    fprintf(stderr, "DEBUG: cupsColorOrder = %d\n", cups.header.cupsColorOrder);
    fprintf(stderr, "DEBUG: cupsColorSpace = %d\n", cups.header.cupsColorSpace);
    fprintf(stderr, "DEBUG: cupsCompression = %d\n", cups.header.cupsCompression);
    fprintf(stderr, "DEBUG: cupsRowCount = %d\n", cups.header.cupsRowCount);
    fprintf(stderr, "DEBUG: cupsRowFeed = %d\n", cups.header.cupsRowFeed);
    fprintf(stderr, "DEBUG: cupsRowStep = %d\n", cups.header.cupsRowStep);

   /*
    * Setup printer driver variables...
    */

    if (cups.page == 0)
      {
	stp_parameter_list_t params;
	int nparams;
	v = stp_allocate_copy(stp_printer_get_printvars(printer));

	stp_set_float_parameter(v, "AppGamma", 1.0);
	for (i = 0; i < stp_option_count; i++)
	  stp_set_float_parameter(v, stp_options[i].iname,
				  stp_options[i].defval / 1000.0);
	stp_set_page_width(v, cups.header.PageSize[0]);
	stp_set_page_height(v, cups.header.PageSize[1]);
	stp_set_outfunc(v, cups_writefunc);
	stp_set_errfunc(v, cups_writefunc);
	stp_set_outdata(v, stdout);
	stp_set_errdata(v, stderr);

	switch (cups.header.cupsColorSpace)
	  {
	  case CUPS_CSPACE_W :
	    stp_set_output_type(v, OUTPUT_GRAY);
	    break;
	  case CUPS_CSPACE_K :
	    stp_set_output_type(v, OUTPUT_GRAY);
	    stp_set_float_parameter(v, "Density", 4.0);
	    break;
	  case CUPS_CSPACE_RGB :
	    stp_set_output_type(v, OUTPUT_COLOR);
	    break;
	  case CUPS_CSPACE_CMYK :
	    stp_set_output_type(v, OUTPUT_RAW_CMYK);
	    break;
	  default :
	    fprintf(stderr, "ERROR: Bad colorspace %d!",
		    cups.header.cupsColorSpace);
	    break;
	  }

	set_special_parameter(v, "DitherAlgorithm", cups.header.cupsRowStep);
	set_special_parameter(v, "Resolution", cups.header.cupsCompression);
	set_special_parameter(v, "ImageOptimization",cups.header.cupsRowCount);

	stp_set_string_parameter(v, "InputSlot", cups.header.MediaClass);
	stp_set_string_parameter(v, "MediaType", cups.header.MediaType);
	stp_set_string_parameter(v, "InkType", cups.header.OutputType);

	fprintf(stderr, "DEBUG: PageSize = %dx%d\n", cups.header.PageSize[0],
		cups.header.PageSize[1]);

	if ((size = stp_get_papersize_by_size(cups.header.PageSize[1],
					      cups.header.PageSize[0])) != NULL)
	  stp_set_string_parameter(v, "PageSize", stp_papersize_get_name(size));
	else
	  fprintf(stderr, "ERROR: Unable to get media size!\n");

	stp_merge_printvars(v, stp_printer_get_printvars(printer));

	params = stp_list_parameters(v);
	nparams = stp_parameter_list_count(params);
	for (i = 0; i < nparams; i++)
	  {
	    const stp_parameter_t *p = stp_parameter_list_param(params, i);
	    switch (p->p_type)
	      {
	      case STP_PARAMETER_TYPE_STRING_LIST:
		fprintf(stderr, "DEBUG: stp_get_%s(v) |%s|\n",
			p->name, stp_get_string_parameter(v, p->name) ?
			stp_get_string_parameter(v, p->name) : "NULL");
		break;
	      case STP_PARAMETER_TYPE_DOUBLE:
		fprintf(stderr, "DEBUG: stp_get_%s(v) |%.3f|\n",
			p->name, stp_get_float_parameter(v, p->name));
		break;
	      case STP_PARAMETER_TYPE_INT:
		fprintf(stderr, "DEBUG: stp_get_%s(v) |%.d|\n",
			p->name, stp_get_int_parameter(v, p->name));
		break;
	      case STP_PARAMETER_TYPE_BOOLEAN:
		fprintf(stderr, "DEBUG: stp_get_%s(v) |%.d|\n",
			p->name, stp_get_boolean_parameter(v, p->name));
		break;
	      default:
		break;
	      }
	  }
	stp_parameter_list_destroy(params);
	stp_set_job_mode(v, STP_JOB_MODE_JOB);
      }

    fprintf(stderr, "DEBUG: stp_get_driver(v) |%s|\n", stp_get_driver(v));
    fprintf(stderr, "DEBUG: stp_get_output_type(v) |%d|\n", stp_get_output_type(v));
    fprintf(stderr, "DEBUG: stp_get_left(v) |%d|\n", stp_get_left(v));
    fprintf(stderr, "DEBUG: stp_get_top(v) |%d|\n", stp_get_top(v));
    fprintf(stderr, "DEBUG: stp_get_page_width(v) |%d|\n", stp_get_page_width(v));
    fprintf(stderr, "DEBUG: stp_get_page_height(v) |%d|\n", stp_get_page_height(v));
    fprintf(stderr, "DEBUG: stp_get_input_color_model(v) |%d|\n", stp_get_input_color_model(v));
    fprintf(stderr, "DEBUG: stp_get_output_color_model(v) |%d|\n", stp_get_output_color_model(v));
    stp_set_page_number(v, cups.page);

    stp_get_media_size(v, &(cups.width), &(cups.height));
    stp_get_imageable_area(v, &(cups.left), &(cups.right),
			   &(cups.bottom), &(cups.top));
    fprintf(stderr, "DEBUG: GIMP-PRINT %d %d %d  %d %d %d\n",
	    cups.width, cups.left, cups.right, cups.height, cups.top, cups.bottom);
    stp_set_width(v, cups.right - cups.left);
    stp_set_height(v, cups.bottom - cups.top);
    stp_set_left(v, cups.left);
    stp_set_top(v, cups.top);
    cups.right = cups.width - cups.right;
    cups.width = cups.width - cups.left - cups.right;
    cups.width = cups.header.HWResolution[0] * cups.width / 72;
    cups.left = cups.header.HWResolution[0] * cups.left / 72;
    cups.right = cups.header.HWResolution[0] * cups.right / 72;

    cups.bottom = cups.height - cups.bottom;
    cups.height = cups.height - cups.top - cups.bottom;
    cups.height = cups.header.HWResolution[1] * cups.height / 72;
    cups.top = cups.header.HWResolution[1] * cups.top / 72;
    cups.bottom = cups.header.HWResolution[1] * cups.bottom / 72;
    fprintf(stderr, "DEBUG: GIMP-PRINT %d %d %d  %d %d %d\n",
	    cups.width, cups.left, cups.right, cups.height, cups.top, cups.bottom);

    /*
     * Print the page...
     */
    if (stp_verify(v))
    {
      signal(SIGTERM, cancel_job);
      if (cups.page == 0)
	stp_start_job(v, &theImage);
      stp_print(v, &theImage);
      fflush(stdout);
    }
    else
      fputs("ERROR: Invalid printer settings!\n", stderr);

   /*
    * Purge any remaining bitmap data...
    */

    if (cups.row < cups.header.cupsHeight)
    {
      if ((buffer = xmalloc(cups.header.cupsBytesPerLine)) == NULL)
        break;

      while (cups.row < cups.header.cupsHeight)
      {
        cupsRasterReadPixels(cups.ras, (unsigned char *)buffer,
	                     cups.header.cupsBytesPerLine);
	cups.row ++;
      }
    }
    cups.page ++;
  }

  if (cups.page > 0)
    stp_end_job(v, &theImage);
  stp_vars_free(v);

 /*
  * Close the raster stream...
  */

  cupsRasterClose(cups.ras);
  if (fd != 0)
    close(fd);

 /*
  * If no pages were printed, send an error message...
  */

  if (cups.page == 0)
    fputs("ERROR: No pages found!\n", stderr);
  else
    fputs("INFO: Ready to print.\n", stderr);

  return (cups.page == 0);
}


/*
 * 'cups_writefunc()' - Write data to a file...
 */

static void
cups_writefunc(void *file, const char *buf, size_t bytes)
{
  FILE *prn = (FILE *)file;
  fwrite(buf, 1, bytes, prn);
}


/*
 * 'cancel_job()' - Cancel the current job...
 */

void
cancel_job(int sig)			/* I - Signal */
{
  (void)sig;

  Image_status = STP_IMAGE_ABORT;
}


/*
 * 'Image_bpp()' - Return the bytes-per-pixel of an image.
 */

static int				/* O - Bytes per pixel */
Image_bpp(stp_image_t *image)		/* I - Image */
{
  cups_image_t	*cups;		/* CUPS image */


  if ((cups = (cups_image_t *)(image->rep)) == NULL)
    return (0);

 /*
  * For now, we only support RGB and grayscale input from the
  * raster filters.
  */

  switch (cups->header.cupsColorSpace)
  {
    default :
        return (1);
    case CUPS_CSPACE_RGB :
        return (3);
    case CUPS_CSPACE_CMYK :
        return (4);
  }
}


/*
 * 'Image_get_appname()' - Get the application we are running.
 */

static const char *				/* O - Application name */
Image_get_appname(stp_image_t *image)		/* I - Image */
{
  (void)image;

  return ("CUPS 1.1.x driver based on GIMP-print");
}


/*
 * 'Image_get_row()' - Get one row of the image.
 */

static void
throwaway_data(int amount, cups_image_t *cups)
{
  unsigned char trash[4096];	/* Throwaway */
  int block_count = amount / 4096;
  int leftover = amount % 4096;
  while (block_count > 0)
    {
      cupsRasterReadPixels(cups->ras, trash, 4096);
      block_count--;
    }
  if (leftover)
    cupsRasterReadPixels(cups->ras, trash, leftover);
}

stp_image_status_t
Image_get_row(stp_image_t   *image,	/* I - Image */
	      unsigned char *data,	/* O - Row */
	      size_t	    byte_limit,	/* I - how many bytes in data */
	      int           row)	/* I - Row number (unused) */
{
  cups_image_t	*cups;			/* CUPS image */
  int		i;			/* Looping var */
  int 		bytes_per_line;
  int		margin;


  if ((cups = (cups_image_t *)(image->rep)) == NULL)
    return STP_IMAGE_ABORT;
  bytes_per_line = cups->width * cups->header.cupsBitsPerPixel / CHAR_BIT;
  margin = cups->header.cupsBytesPerLine - bytes_per_line;

  if (cups->row < cups->header.cupsHeight)
  {
    fprintf(stderr, "DEBUG: GIMP-PRINT reading %d %d\n",
	    bytes_per_line, cups->row);
    cupsRasterReadPixels(cups->ras, data, bytes_per_line);
    cups->row ++;
    if (margin) {
      fprintf(stderr, "DEBUG: GIMP-PRINT tossing right %d\n", margin);
      throwaway_data(margin, cups);
    }

   /*
    * Invert black data for monochrome output...
    */

    if (cups->header.cupsColorSpace == CUPS_CSPACE_K)
      for (i = bytes_per_line; i > 0; i --, data ++)
        *data = ((1 << CHAR_BIT) - 1) - *data;
  }
  else
    {
      if (cups->header.cupsColorSpace == CUPS_CSPACE_CMYK)
	memset(data, 0, bytes_per_line);
      else
	memset(data, ((1 << CHAR_BIT) - 1), bytes_per_line);
    }
  return Image_status;
}


/*
 * 'Image_height()' - Return the height of an image.
 */

static int				/* O - Height in pixels */
Image_height(stp_image_t *image)	/* I - Image */
{
  cups_image_t	*cups;		/* CUPS image */


  if ((cups = (cups_image_t *)(image->rep)) == NULL)
    return (0);

  fprintf(stderr, "DEBUG: GIMP-PRINT: Image_height %d\n", cups->height);
  return (cups->height);
}


/*
 * 'Image_init()' - Initialize an image.
 */

static void
Image_init(stp_image_t *image)		/* I - Image */
{
  (void)image;
}

/*
 * 'Image_note_progress()' - Notify the user of our progress.
 */

void
Image_note_progress(stp_image_t *image,	/* I - Image */
		    double current,	/* I - Current progress */
		    double total)	/* I - Maximum progress */
{
  cups_image_t	*cups;		/* CUPS image */

  if ((cups = (cups_image_t *)(image->rep)) == NULL)
    return;

  fprintf(stderr, "INFO: Printing page %d, %.0f%%\n",
          cups->page, 100.0 * current / total);
}

/*
 * 'Image_progress_conclude()' - Close the progress display.
 */

static void
Image_progress_conclude(stp_image_t *image)	/* I - Image */
{
  cups_image_t	*cups;		/* CUPS image */


  if ((cups = (cups_image_t *)(image->rep)) == NULL)
    return;

  fprintf(stderr, "INFO: Finished page %d...\n", cups->page);
}

/*
 * 'Image_progress_init()' - Initialize progress display.
 */

static void
Image_progress_init(stp_image_t *image)/* I - Image */
{
  cups_image_t	*cups;		/* CUPS image */


  if ((cups = (cups_image_t *)(image->rep)) == NULL)
    return;

  fprintf(stderr, "INFO: Starting page %d...\n", cups->page);
}

/*
 * 'Image_width()' - Return the width of an image.
 */

static int				/* O - Width in pixels */
Image_width(stp_image_t *image)	/* I - Image */
{
  cups_image_t	*cups;		/* CUPS image */


  if ((cups = (cups_image_t *)(image->rep)) == NULL)
    return (0);

  fprintf(stderr, "DEBUG: GIMP-PRINT: Image_width %d\n", cups->width);
  return (cups->width);
}


/*
 * End of "$Id: rastertoprinter.c,v 1.47 2003/01/20 02:08:37 rlk Exp $".
 */
