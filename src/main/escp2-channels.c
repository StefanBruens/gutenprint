/*
 * "$Id: escp2-channels.c,v 1.10 2003/06/07 20:15:24 rlk Exp $"
 *
 *   Print plug-in EPSON ESC/P2 driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include "print-escp2.h"

static const char standard_sat_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
"1.00 1.10 1.20 1.30 1.40 1.50 1.60 1.70 "  /* C */
"1.80 1.90 1.90 1.90 1.70 1.50 1.30 1.10 "  /* B */
"1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* M */
"1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* R */
"1.00 1.00 1.00 1.10 1.20 1.30 1.40 1.50 "  /* Y */
"1.50 1.40 1.30 1.20 1.10 1.00 1.00 1.00 "  /* G */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char standard_lum_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
"0.50 0.60 0.70 0.80 0.90 0.86 0.82 0.79 "  /* C */
"0.78 0.80 0.83 0.87 0.90 0.95 1.05 1.15 "  /* B */
"1.30 1.25 1.20 1.15 1.12 1.09 1.06 1.03 "  /* M */
"1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* R */
"1.00 0.90 0.80 0.70 0.65 0.60 0.55 0.52 "  /* Y */
"0.48 0.47 0.47 0.49 0.49 0.49 0.52 0.51 "  /* G */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char standard_hue_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"-6\" upper-bound=\"6\">\n"
"0.00 0.05 0.04 0.01 -.03 -.10 -.18 -.26 "  /* C */
"-.35 -.43 -.40 -.32 -.25 -.18 -.10 -.07 "  /* B */
"0.00 -.04 -.09 -.13 -.18 -.23 -.27 -.31 "  /* M */
"-.35 -.38 -.30 -.23 -.15 -.08 0.00 -.02 "  /* R */
"0.00 0.08 0.10 0.08 0.05 0.03 -.03 -.12 "  /* Y */
"-.20 0.17 -.20 -.17 -.15 -.12 -.10 -.08 "  /* G */
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";


#define DECLARE_INK_CHANNEL(name)				\
static const ink_channel_t name##_channels =			\
{								\
  #name,							\
  name##_subchannels,						\
  sizeof(name##_subchannels) / sizeof(physical_subchannel_t),	\
}

static const physical_subchannel_t standard_black_subchannels[] =
{
  { 0, -1, 0, "BlackDensity", NULL }
};

DECLARE_INK_CHANNEL(standard_black);

static const physical_subchannel_t x80_black_subchannels[] =
{
  { 0, -1, 48, "BlackDensity", NULL }
};

DECLARE_INK_CHANNEL(x80_black);

static const physical_subchannel_t c80_black_subchannels[] =
{
  { 0, -1, 0, "BlackDensity", NULL }
};

DECLARE_INK_CHANNEL(c80_black);

static const physical_subchannel_t standard_cyan_subchannels[] =
{
  { 2, -1, 0, "CyanDensity", NULL }
};

DECLARE_INK_CHANNEL(standard_cyan);

static const physical_subchannel_t f360_standard_cyan_subchannels[] =
{
  { 2, -1, 1, "CyanDensity", NULL }
};

DECLARE_INK_CHANNEL(f360_standard_cyan);

static const physical_subchannel_t x80_cyan_subchannels[] =
{
  { 2, -1, 96, "CyanDensity", NULL }
};

DECLARE_INK_CHANNEL(x80_cyan);

static const physical_subchannel_t c80_cyan_subchannels[] =
{
  { 2, -1, 0, "CyanDensity", NULL }
};

DECLARE_INK_CHANNEL(c80_cyan);

static const physical_subchannel_t standard_magenta_subchannels[] =
{
  { 1, -1, 0, "MagentaDensity", NULL }
};

DECLARE_INK_CHANNEL(standard_magenta);

static const physical_subchannel_t f360_standard_magenta_subchannels[] =
{
  { 1, -1, 1, "MagentaDensity", NULL }
};

DECLARE_INK_CHANNEL(f360_standard_magenta);

static const physical_subchannel_t x80_magenta_subchannels[] =
{
  { 1, -1, 48, "MagentaDensity", NULL }
};

DECLARE_INK_CHANNEL(x80_magenta);

static const physical_subchannel_t c80_magenta_subchannels[] =
{
  { 1, -1, 120, "MagentaDensity", NULL }
};

DECLARE_INK_CHANNEL(c80_magenta);

static const physical_subchannel_t standard_yellow_subchannels[] =
{
  { 4, -1, 0, "YellowDensity", NULL }
};

DECLARE_INK_CHANNEL(standard_yellow);

static const physical_subchannel_t x80_yellow_subchannels[] =
{
  { 4, -1, 0, "YellowDensity", NULL }
};

DECLARE_INK_CHANNEL(x80_yellow);

static const physical_subchannel_t c80_yellow_subchannels[] =
{
  { 4, -1, 240, "YellowDensity", NULL }
};

DECLARE_INK_CHANNEL(c80_yellow);

static const physical_subchannel_t photo_black_subchannels[] =
{
  { 0, 0, 0, "BlackDensity", NULL }
};

DECLARE_INK_CHANNEL(photo_black);

static const physical_subchannel_t f360_photo_black_subchannels[] =
{
  { 0, 0, 1, "BlackDensity", NULL }
};

DECLARE_INK_CHANNEL(f360_photo_black);

static const physical_subchannel_t extended_black_subchannels[] =
{
  { 0, 1, 0, "BlackDensity", NULL }
};

DECLARE_INK_CHANNEL(extended_black);

static const physical_subchannel_t photo_cyan_subchannels[] =
{
  { 2, 0, 0, "CyanDensity", NULL },
  { 2, 1, 0, "CyanDensity", "LightCyanTransition" }
};

DECLARE_INK_CHANNEL(photo_cyan);

static const physical_subchannel_t extended_cyan_subchannels[] =
{
  { 2, 1, 0, "CyanDensity", NULL }
};

DECLARE_INK_CHANNEL(extended_cyan);

static const physical_subchannel_t photo_magenta_subchannels[] =
{
  { 1, 0, 0, "MagentaDensity", NULL },
  { 1, 1, 0, "MagentaDensity", "LightMagentaTransition" }
};

DECLARE_INK_CHANNEL(photo_magenta);

static const physical_subchannel_t extended_magenta_subchannels[] =
{
  { 1, 1, 0, "MagentaDensity", NULL }
};

DECLARE_INK_CHANNEL(extended_magenta);

static const physical_subchannel_t photo_yellow_subchannels[] =
{
  { 4, 0, 0, "YellowDensity", NULL }
};

DECLARE_INK_CHANNEL(photo_yellow);

static const physical_subchannel_t f360_photo_yellow_subchannels[] =
{
  { 4, 0, 1, "YellowDensity", NULL }
};

DECLARE_INK_CHANNEL(f360_photo_yellow);

static const physical_subchannel_t j_extended_yellow_subchannels[] =
{
  { 4, 2, 0, "YellowDensity", NULL }
};

DECLARE_INK_CHANNEL(j_extended_yellow);

/* For Japanese 7-color printers, with dark yellow */
static const physical_subchannel_t photo2_yellow_subchannels[] =
{
  { 4, 2, 0, "YellowDensity", NULL },
  { 4, 0, 0, "YellowDensity", "DarkYellowTransition" }
};

DECLARE_INK_CHANNEL(photo2_yellow);

static const physical_subchannel_t f360_photo2_yellow_subchannels[] =
{
  { 4, 2, 0, "YellowDensity", NULL },
  { 4, 0, 1, "YellowDensity", "DarkYellowTransition" }
};

DECLARE_INK_CHANNEL(f360_photo2_yellow);

static const physical_subchannel_t photo2_black_subchannels[] =
{
  { 0, 0, 0, "BlackDensity", NULL },
  { 0, 1, 0, "BlackDensity", "GrayTransition" }
};

DECLARE_INK_CHANNEL(photo2_black);

static const physical_subchannel_t f360_photo2_black_subchannels[] =
{
  { 0, 0, 1, "BlackDensity", NULL },
  { 0, 1, 0, "BlackDensity", "GrayTransition" }
};

DECLARE_INK_CHANNEL(f360_photo2_black);

static const physical_subchannel_t quadtone_subchannels[] =
{
  { 0, -1, 0, "BlackDensity", NULL },
  { 2, -1, 0, "BlackDensity", "Gray3Transition" },
  { 1, -1, 0, "BlackDensity", "Gray2Transition" },
  { 4, -1, 0, "BlackDensity", "Gray1Transition" },
};

DECLARE_INK_CHANNEL(quadtone);

static const physical_subchannel_t c80_quadtone_subchannels[] =
{
  { 0, -1, 0, "BlackDensity", NULL },
  { 2, -1, 0, "BlackDensity", "Gray3Transition" },
  { 1, -1, 120, "BlackDensity", "Gray2Transition" },
  { 4, -1, 240, "BlackDensity", "Gray1Transition" },
};

DECLARE_INK_CHANNEL(c80_quadtone);

static const physical_subchannel_t f360_photo_cyan_subchannels[] =
{
  { 2, 0, 1, "CyanDensity", NULL },
  { 2, 1, 0, "CyanDensity", "LightCyanTransition" }
};

DECLARE_INK_CHANNEL(f360_photo_cyan);

static const physical_subchannel_t f360_photo_magenta_subchannels[] =
{
  { 1, 0, 1, "MagentaDensity", NULL },
  { 1, 1, 0, "MagentaDensity", "LightMagentaTransition" }
};

DECLARE_INK_CHANNEL(f360_photo_magenta);


/*
 ****************************************************************
 *                                                              *
 * Two shade gray                                               *
 *                                                              *
 ****************************************************************
 */

static const escp2_inkname_t two_color_grayscale_inkset =
{
  "Gray2", N_("Two Level Grayscale"), 0, 1, INKSET_CcMmYKk,
  &stpi_escp2_standard_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &photo2_black_channels
  }
};

static const escp2_inkname_t ultra_photo_two_color_grayscale_inkset =
{
  "Gray2", N_("Two Level Grayscale"), 0, 1, INKSET_CcMmYKk,
  &stpi_escp2_ultrachrome_photo_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &photo2_black_channels
  }
};

static const escp2_inkname_t f360_ultra_photo_two_color_grayscale_inkset =
{
  "Gray2", N_("Two Level Grayscale"), 0, 1, INKSET_CcMmYKk,
  &stpi_escp2_ultrachrome_photo_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &f360_photo2_black_channels
  }
};

static const escp2_inkname_t f360_two_color_grayscale_inkset =
{
  "Gray2", N_("Two Level Grayscale"), 0, 1, INKSET_CcMmYKk,
  &stpi_escp2_standard_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &f360_photo2_black_channels
  }
};


/*
 ****************************************************************
 *                                                              *
 * Quadtone gray                                                *
 *                                                              *
 ****************************************************************
 */

static const escp2_inkname_t generic_quadtone_inkset =
{
  "Quadtone", N_("Quadtone"), 0, 1, INKSET_QUADTONE,
  &stpi_escp2_standard_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &quadtone_channels
  }
};

static const escp2_inkname_t c80_generic_quadtone_inkset =
{
  "Quadtone", N_("Quadtone"), 0, 1, INKSET_QUADTONE,
  &stpi_escp2_standard_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &c80_quadtone_channels
  }
};



/*
 ****************************************************************
 *                                                              *
 * Three color CMY                                              *
 *                                                              *
 ****************************************************************
 */

static const escp2_inkname_t three_color_composite_inkset =
{
  "RGB", N_("Three Color Composite"), 1, 4, INKSET_CMYK,
  &stpi_escp2_standard_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    NULL, &standard_cyan_channels,
    &standard_magenta_channels, &standard_yellow_channels
  }
};

static const escp2_inkname_t x80_three_color_composite_inkset =
{
  "RGB", N_("Three Color Composite"), 1, 4, INKSET_CMYK,
  &stpi_escp2_standard_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    NULL, &x80_cyan_channels,
    &x80_magenta_channels, &x80_yellow_channels
  }
};

static const escp2_inkname_t c80_three_color_composite_inkset =
{
  "RGB", N_("Three Color Composite"), 1, 4, INKSET_CMYK,
  &stpi_escp2_standard_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    NULL, &c80_cyan_channels,
    &c80_magenta_channels, &c80_yellow_channels
  }
};


/*
 ****************************************************************
 *                                                              *
 * Four color CMYK                                              *
 *                                                              *
 ****************************************************************
 */

static const escp2_inkname_t four_color_standard_inkset =
{
  "CMYK", N_("Four Color Standard"), 1, 4, INKSET_CMYK,
  &stpi_escp2_standard_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    &standard_black_channels, &standard_cyan_channels,
    &standard_magenta_channels, &standard_yellow_channels
  }
};

static const escp2_inkname_t x80_four_color_standard_inkset =
{
  "CMYK", N_("Four Color Standard"), 1, 4, INKSET_CMYK,
  &stpi_escp2_standard_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    &x80_black_channels, &x80_cyan_channels,
    &x80_magenta_channels, &x80_yellow_channels
  }
};

static const escp2_inkname_t c80_four_color_standard_inkset =
{
  "CMYK", N_("Four Color Standard"), 1, 4, INKSET_CMYK,
  &stpi_escp2_standard_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    &c80_black_channels, &c80_cyan_channels,
    &c80_magenta_channels, &c80_yellow_channels
  }
};


/*
 ****************************************************************
 *                                                              *
 * Five color CcMmY                                             *
 *                                                              *
 ****************************************************************
 */

static const escp2_inkname_t five_color_photo_composite_inkset =
{
  "PhotoCMY", N_("Five Color Photo Composite"), 1, 4, INKSET_CcMmYK,
  &stpi_escp2_photo_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    NULL, &photo_cyan_channels,
    &photo_magenta_channels, &photo_yellow_channels
  }
};

static const escp2_inkname_t ultra_photo_five_color_photo_composite_inkset =
{
  "PhotoCMY", N_("Five Color Photo Composite"), 1, 4, INKSET_CcMmYK,
  &stpi_escp2_ultrachrome_photo_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    NULL, &photo_cyan_channels,
    &photo_magenta_channels, &photo_yellow_channels
  }
};

static const escp2_inkname_t f360_ultra_photo_five_color_photo_composite_inkset =
{
  "PhotoCMY", N_("Five Color Photo Composite"), 1, 4, INKSET_CcMmYK,
  &stpi_escp2_ultrachrome_photo_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    NULL, &f360_photo_cyan_channels,
    &f360_photo_magenta_channels, &f360_photo_yellow_channels
  }
};

static const escp2_inkname_t f360_five_color_photo_composite_inkset =
{
  "PhotoCMY", N_("Five Color Photo Composite"), 1, 4, INKSET_CcMmYK,
  &stpi_escp2_photo_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    NULL, &f360_photo_cyan_channels,
    &f360_photo_magenta_channels, &f360_photo_yellow_channels
  }
};



/*
 ****************************************************************
 *                                                              *
 * Six color CcMmYK                                             *
 *                                                              *
 ****************************************************************
 */

static const escp2_inkname_t six_color_photo_inkset =
{
  "PhotoCMYK", N_("Six Color Photo"), 1, 4, INKSET_CcMmYK,
  &stpi_escp2_photo_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    &photo_black_channels, &photo_cyan_channels,
    &photo_magenta_channels, &photo_yellow_channels
  }
};

static const escp2_inkname_t ultra_photo_six_color_photo_inkset =
{
  "PhotoCMYK", N_("Six Color Photo"), 1, 4, INKSET_CcMmYK,
  &stpi_escp2_ultrachrome_photo_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    &photo_black_channels, &photo_cyan_channels,
    &photo_magenta_channels, &photo_yellow_channels
  }
};

static const escp2_inkname_t f360_ultra_photo_six_color_photo_inkset =
{
  "PhotoCMYK", N_("Six Color Photo"), 1, 4, INKSET_CcMmYK,
  &stpi_escp2_ultrachrome_photo_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    &f360_photo_black_channels, &f360_photo_cyan_channels,
    &f360_photo_magenta_channels, &f360_photo_yellow_channels
  }
};

static const escp2_inkname_t f360_six_color_photo_inkset =
{
  "PhotoCMYK", N_("Six Color Photo"), 1, 4, INKSET_CcMmYK,
  &stpi_escp2_sp960_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    &f360_photo_black_channels, &f360_photo_cyan_channels,
    &f360_photo_magenta_channels, &f360_photo_yellow_channels
  }
};



/*
 ****************************************************************
 *                                                              *
 * Six color CcMmYy (Japan)                                     *
 *                                                              *
 ****************************************************************
 */

static const escp2_inkname_t j_six_color_enhanced_composite_inkset =
{
  "PhotoEnhanceJ", N_("Six Color Enhanced Composite"), 1, 4, INKSET_CcMmYyK,
  &stpi_escp2_standard_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    NULL, &photo_cyan_channels,
    &photo_magenta_channels, &photo2_yellow_channels
  }
};

static const escp2_inkname_t f360_j_six_color_enhanced_composite_inkset =
{
  "PhotoEnhanceJ", N_("Six Color Enhanced Composite"), 1, 4, INKSET_CcMmYyK,
  &stpi_escp2_standard_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    NULL, &f360_photo_cyan_channels,
    &f360_photo_magenta_channels, &f360_photo2_yellow_channels
  }
};


/*
 ****************************************************************
 *                                                              *
 * Seven color CcMmYKk                                          *
 *                                                              *
 ****************************************************************
 */

static const escp2_inkname_t seven_color_enhanced_inkset =
{
  "PhotoCMYK7", N_("Seven Color Photo"), 1, 4, INKSET_CcMmYKk,
  &stpi_escp2_photo_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    &photo2_black_channels, &photo_cyan_channels,
    &photo_magenta_channels, &photo_yellow_channels
  }
};

static const escp2_inkname_t f360_seven_color_enhanced_inkset =
{
  "PhotoCMYK7", N_("Seven Color Photo"), 1, 4, INKSET_CcMmYKk,
  &stpi_escp2_photo_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    &f360_photo2_black_channels, &f360_photo_cyan_channels,
    &f360_photo_magenta_channels, &f360_photo_yellow_channels
  }
};

static const escp2_inkname_t f360_ultra_photo_seven_color_enhanced_inkset =
{
  "PhotoCMYK7", N_("Seven Color Photo"), 1, 4, INKSET_CcMmYKk,
  &stpi_escp2_ultrachrome_photo_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    &f360_photo2_black_channels, &f360_photo_cyan_channels,
    &f360_photo_magenta_channels, &f360_photo_yellow_channels
  }
};

static const escp2_inkname_t ultra_photo_seven_color_enhanced_inkset =
{
  "PhotoCMYK7", N_("Seven Color Photo"), 1, 4, INKSET_CcMmYKk,
  &stpi_escp2_ultrachrome_photo_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    &photo2_black_channels, &photo_cyan_channels,
    &photo_magenta_channels, &photo_yellow_channels
  }
};

/*
 ****************************************************************
 *                                                              *
 * Seven color CcMmYyK (Japan)                                  *
 *                                                              *
 ****************************************************************
 */

static const escp2_inkname_t j_seven_color_enhanced_inkset =
{
  "Photo7J", N_("Seven Color Enhanced"), 1, 4, INKSET_CcMmYyK,
  &stpi_escp2_photo_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    &photo_black_channels, &photo_cyan_channels,
    &photo_magenta_channels, &photo2_yellow_channels
  }
};

static const escp2_inkname_t f360_j_seven_color_enhanced_inkset =
{
  "Photo7J", N_("Seven Color Photo"), 1, 4, INKSET_CcMmYKk,
  &stpi_escp2_photo_paper_adjustment_list,
  standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
  {
    &f360_photo_black_channels, &f360_photo_cyan_channels,
    &f360_photo_magenta_channels, &f360_photo2_yellow_channels
  }
};



/*
 ****************************************************************
 *                                                              *
 * Extended (raw)                                               *
 *                                                              *
 ****************************************************************
 */

static const escp2_inkname_t one_color_extended_inkset =
{
  "PhysicalBlack", N_("One Color Raw"), 0, 1, INKSET_EXTENDED,
  &stpi_escp2_standard_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &standard_black_channels
  }
};

static const escp2_inkname_t two_color_extended_inkset =
{
  "PhysicalBlack2", N_("Two Color Raw"), 1, 2, INKSET_EXTENDED,
  &stpi_escp2_standard_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &photo_black_channels, &extended_black_channels,
  }
};

static const escp2_inkname_t f360_ultra_photo_two_color_extended_inkset =
{
  "PhysicalBlack2", N_("Two Color Raw"), 1, 2, INKSET_EXTENDED,
  &stpi_escp2_ultrachrome_photo_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &f360_photo_black_channels, &extended_black_channels,
  }
};


static const escp2_inkname_t f360_two_color_extended_inkset =
{
  "PhysicalBlack2", N_("Two Color Raw"), 1, 2, INKSET_EXTENDED,
  &stpi_escp2_standard_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &f360_photo_black_channels, &extended_black_channels,
  }
};

static const escp2_inkname_t ultra_photo_two_color_extended_inkset =
{
  "PhysicalBlack2", N_("Two Color Raw"), 1, 2, INKSET_EXTENDED,
  &stpi_escp2_ultrachrome_photo_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &photo_black_channels, &extended_black_channels,
  }
};

static const escp2_inkname_t three_color_extended_inkset =
{
  "PhysicalCMY", N_("Three Color Raw"), 1, 3, INKSET_EXTENDED,
  &stpi_escp2_standard_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &standard_cyan_channels, &standard_magenta_channels,
    &standard_yellow_channels
  }
};

static const escp2_inkname_t x80_three_color_extended_inkset =
{
  "PhysicalCMY", N_("Three Color Raw"), 1, 3, INKSET_EXTENDED,
  &stpi_escp2_standard_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &x80_cyan_channels, &x80_magenta_channels, &x80_yellow_channels
  }
};

static const escp2_inkname_t c80_three_color_extended_inkset =
{
  "PhysicalCMY", N_("Three Color Raw"), 1, 3, INKSET_EXTENDED,
  &stpi_escp2_standard_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &c80_cyan_channels, &c80_magenta_channels, &c80_yellow_channels
  }
};

static const escp2_inkname_t four_color_extended_inkset =
{
  "PhysicalCMYK", N_("Four Color Raw"), 1, 4, INKSET_EXTENDED,
  &stpi_escp2_standard_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &standard_black_channels, &standard_cyan_channels,
    &standard_magenta_channels, &standard_yellow_channels
  }
};

static const escp2_inkname_t x80_four_color_extended_inkset =
{
  "PhysicalCMYK", N_("Four Color Raw"), 1, 4, INKSET_EXTENDED,
  &stpi_escp2_standard_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &x80_black_channels, &x80_cyan_channels,
    &x80_magenta_channels, &x80_yellow_channels
  }
};

static const escp2_inkname_t c80_four_color_extended_inkset =
{
  "PhysicalCMYK", N_("Four Color Raw"), 1, 4, INKSET_EXTENDED,
  &stpi_escp2_standard_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &c80_black_channels, &c80_cyan_channels,
    &c80_magenta_channels, &c80_yellow_channels
  }
};

static const escp2_inkname_t five_color_extended_inkset =
{
  "PhysicalCcMmY", N_("Five Color Raw"), 1, 5, INKSET_EXTENDED,
  &stpi_escp2_standard_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &standard_cyan_channels, &extended_cyan_channels,
    &standard_magenta_channels, &extended_magenta_channels,
    &photo_yellow_channels
  }
};

static const escp2_inkname_t f360_ultra_photo_five_color_extended_inkset =
{
  "PhysicalCcMmY", N_("Five Color Raw"), 1, 5, INKSET_EXTENDED,
  &stpi_escp2_ultrachrome_photo_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &f360_standard_cyan_channels, &extended_cyan_channels,
    &f360_standard_magenta_channels, &extended_magenta_channels,
    &f360_photo_yellow_channels
  }
};


static const escp2_inkname_t f360_five_color_extended_inkset =
{
  "PhysicalCcMmY", N_("Five Color Raw"), 1, 5, INKSET_EXTENDED,
  &stpi_escp2_standard_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &f360_standard_cyan_channels, &extended_cyan_channels,
    &f360_standard_magenta_channels, &extended_magenta_channels,
    &f360_photo_yellow_channels
  }
};

static const escp2_inkname_t ultra_photo_five_color_extended_inkset =
{
  "PhysicalCcMmY", N_("Five Color Raw"), 1, 5, INKSET_EXTENDED,
  &stpi_escp2_ultrachrome_photo_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &standard_cyan_channels, &extended_cyan_channels,
    &standard_magenta_channels, &extended_magenta_channels,
    &photo_yellow_channels
  }
};

static const escp2_inkname_t six_color_extended_inkset =
{
  "PhysicalCcMmYK", N_("Six Color Raw"), 1, 6, INKSET_EXTENDED,
  &stpi_escp2_standard_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &photo_black_channels,
    &standard_cyan_channels, &extended_cyan_channels,
    &standard_magenta_channels, &extended_magenta_channels,
    &photo_yellow_channels
  }
};

static const escp2_inkname_t f360_ultra_photo_six_color_extended_inkset =
{
  "PhysicalCcMmYK", N_("Six Color Raw"), 1, 6, INKSET_EXTENDED,
  &stpi_escp2_ultrachrome_photo_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &f360_photo_black_channels,
    &f360_standard_cyan_channels, &extended_cyan_channels,
    &f360_standard_magenta_channels, &extended_magenta_channels,
    &f360_photo_yellow_channels
  }
};


static const escp2_inkname_t f360_six_color_extended_inkset =
{
  "PhysicalCcMmYK", N_("Six Color Raw"), 1, 6, INKSET_EXTENDED,
  &stpi_escp2_standard_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &f360_photo_black_channels,
    &f360_standard_cyan_channels, &extended_cyan_channels,
    &f360_standard_magenta_channels, &extended_magenta_channels,
    &f360_photo_yellow_channels
  }
};

static const escp2_inkname_t ultra_photo_six_color_extended_inkset =
{
  "PhysicalCcMmYK", N_("Six Color Raw"), 1, 6, INKSET_EXTENDED,
  &stpi_escp2_ultrachrome_photo_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &photo_black_channels,
    &standard_cyan_channels, &extended_cyan_channels,
    &standard_magenta_channels, &extended_magenta_channels,
    &photo_yellow_channels
  }
};

static const escp2_inkname_t j_seven_color_extended_inkset =
{
  "PhysicalCcMmYyK", N_("Seven Color Raw"), 1, 7, INKSET_EXTENDED,
  &stpi_escp2_standard_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &photo_black_channels,
    &standard_cyan_channels, &extended_cyan_channels,
    &standard_magenta_channels, &extended_magenta_channels,
    &photo_yellow_channels, &j_extended_yellow_channels,
  }
};

static const escp2_inkname_t seven_color_extended_inkset =
{
  "PhysicalCcMmYKk", N_("Seven Color Raw"), 1, 7, INKSET_EXTENDED,
  &stpi_escp2_standard_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &photo_black_channels, &extended_black_channels,
    &standard_cyan_channels, &extended_cyan_channels,
    &standard_magenta_channels, &extended_magenta_channels,
    &photo_yellow_channels
  }
};

static const escp2_inkname_t f360_ultra_photo_seven_color_extended_inkset =
{
  "PhysicalCcMmYKk", N_("Seven Color Raw"), 1, 7, INKSET_EXTENDED,
  &stpi_escp2_ultrachrome_photo_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &f360_photo_black_channels, &extended_black_channels,
    &f360_standard_cyan_channels, &extended_cyan_channels,
    &f360_standard_magenta_channels, &extended_magenta_channels,
    &f360_photo_yellow_channels
  }
};


static const escp2_inkname_t f360_seven_color_extended_inkset =
{
  "PhysicalCcMmYKk", N_("Seven Color Raw"), 1, 7, INKSET_EXTENDED,
  &stpi_escp2_standard_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &f360_photo_black_channels, &extended_black_channels,
    &f360_standard_cyan_channels, &extended_cyan_channels,
    &f360_standard_magenta_channels, &extended_magenta_channels,
    &f360_photo_yellow_channels
  }
};

static const escp2_inkname_t ultra_photo_seven_color_extended_inkset =
{
  "PhysicalCcMmYKk", N_("Seven Color Raw"), 1, 7, INKSET_EXTENDED,
  &stpi_escp2_ultrachrome_photo_paper_adjustment_list,
  NULL, NULL, NULL,
  {
    &photo_black_channels, &extended_black_channels,
    &standard_cyan_channels, &extended_cyan_channels,
    &standard_magenta_channels, &extended_magenta_channels,
    &photo_yellow_channels
  }
};

shade_set_t standard_shades =
{
  { 1, { 1.0 }},		/* K */
  { 1, { 1.0 }},		/* C */
  { 1, { 1.0 }},		/* M */
  { 1, { 1.0 }},		/* Y */
  { 1, { 1.0 }},		/* Extended 5 */
  { 1, { 1.0 }},		/* Extended 6 */
  { 1, { 1.0 }}			/* Extended 7 */
};

shade_set_t gen1_shades =	/* Stylus 750 and older */
{
  { 1, { 1.0 }},
  { 2, { 1.0, 0.305 }},
  { 2, { 1.0, 0.315 }},
  { 1, { 1.0 }},
  { 1, { 1.0 }},
  { 1, { 1.0 }},
  { 1, { 1.0 }},
};

shade_set_t gen2_shades =	/* Stylus 870 and newer */
{
  { 1, { 1.0 }},
  { 2, { 1.0, 0.26 }},
  { 2, { 1.0, 0.31 }},
  { 1, { 1.0 }},
  { 1, { 1.0 }},
  { 1, { 1.0 }},
  { 1, { 1.0 }},
};

shade_set_t esp960_shades =	/* Epson 950/960/PM-950C/PM-970C */
{
  { 1, { 1.0 }},
  { 2, { 1.0, 0.32 }},
  { 2, { 1.0, 0.35 }},
  { 2, { 1.0, 0.5 }},
  { 1, { 1.0 }},
  { 1, { 1.0 }},
  { 1, { 1.0 }},
};

shade_set_t stp2200_shades =	/* Stylus Photo 2200 */
{
  { 1, { 1.0 }},
  { 2, { 1.0, 0.227 }},		/* Just a guess */
  { 2, { 1.0, 0.227 }},		/* Just a guess */
  { 1, { 1.0 }},
  { 1, { 1.0 }},
  { 1, { 1.0 }},
  { 1, { 1.0 }},
};

shade_set_t ultrachrome_photo_shades =	/* Ultrachrome with photo black ink */
{
  { 2, { 1.0, 0.48 }},
  { 2, { 1.0, 0.32 }},
  { 2, { 1.0, 0.35 }},
  { 1, { 1.0 }},
  { 1, { 1.0 }},
  { 1, { 1.0 }},
  { 1, { 1.0 }},
};

shade_set_t quadtone_shades =	/* Some kind of quadtone ink */
{
  { 4, { 1.0, 0.75, 0.5, 0.25 }},
  { 1, { 1.0 }},
  { 1, { 1.0 }},
  { 1, { 1.0 }},
  { 1, { 1.0 }},
  { 1, { 1.0 }},
  { 1, { 1.0 }},
};

#define DECLARE_INKLIST(name, inks, text, papers, shades)	\
static const inklist_t name##_inklist =				\
{								\
  #name,							\
  text,								\
  inks##_ink_types,						\
  &stpi_escp2_##papers##_paper_list,				\
  &shades##_shades,						\
  sizeof(inks##_ink_types) / sizeof(escp2_inkname_t *),		\
}


static const escp2_inkname_t *const cmy_ink_types[] =
{
  &three_color_composite_inkset
};

DECLARE_INKLIST(cmy, cmy, N_("EPSON Standard Inks"), standard, standard);


static const escp2_inkname_t *const standard_ink_types[] =
{
  &four_color_standard_inkset,
  &three_color_composite_inkset,
  &one_color_extended_inkset,
  &three_color_extended_inkset,
  &four_color_extended_inkset,
};

DECLARE_INKLIST(standard, standard, N_("EPSON Standard Inks"), standard, standard);

static const escp2_inkname_t *const quadtone_ink_types[] =
{
  &generic_quadtone_inkset,
};

DECLARE_INKLIST(quadtone, quadtone, N_("Quadtone"), standard, quadtone);

static const escp2_inkname_t *const c80_ink_types[] =
{
  &c80_four_color_standard_inkset,
  &c80_three_color_composite_inkset,
  &one_color_extended_inkset,
  &c80_three_color_extended_inkset,
  &c80_four_color_extended_inkset,
};

DECLARE_INKLIST(c80, c80, N_("EPSON Standard Inks"), c80, standard);

static const escp2_inkname_t *const c80_quadtone_ink_types[] =
{
  &c80_generic_quadtone_inkset,
};

DECLARE_INKLIST(c80_quadtone, c80_quadtone, N_("Quadtone"), standard, quadtone);

static const escp2_inkname_t *const x80_ink_types[] =
{
  &x80_four_color_standard_inkset,
  &x80_three_color_composite_inkset,
  &one_color_extended_inkset,
  &x80_three_color_extended_inkset,
  &x80_four_color_extended_inkset,
};

DECLARE_INKLIST(x80, x80, N_("EPSON Standard Inks"), standard, standard);

static const escp2_inkname_t *const photo_ink_types[] =
{
  &six_color_photo_inkset,
  &five_color_photo_composite_inkset,
  &four_color_standard_inkset,
  &three_color_composite_inkset,
  &one_color_extended_inkset,
  &three_color_extended_inkset,
  &four_color_extended_inkset,
  &five_color_extended_inkset,
  &six_color_extended_inkset,
};

DECLARE_INKLIST(gen1, photo, N_("EPSON Standard Inks"), standard, gen1);
DECLARE_INKLIST(gen2, photo, N_("EPSON Standard Inks"), standard, gen2);
DECLARE_INKLIST(pigment, photo, N_("EPSON Standard Inks"), standard, stp2200);

static const escp2_inkname_t *const f360_photo_ink_types[] =
{
  &f360_six_color_photo_inkset,
  &f360_five_color_photo_composite_inkset,
  &four_color_standard_inkset,
  &three_color_composite_inkset,
  &one_color_extended_inkset,
  &three_color_extended_inkset,
  &four_color_extended_inkset,
  &f360_five_color_extended_inkset,
  &f360_six_color_extended_inkset,
};

DECLARE_INKLIST(f360_photo, f360_photo, N_("EPSON Standard Inks"), standard, esp960);

static const escp2_inkname_t *const f360_photo7_japan_ink_types[] =
{
  &f360_j_seven_color_enhanced_inkset,
  &f360_j_six_color_enhanced_composite_inkset,
  &f360_six_color_photo_inkset,
  &f360_five_color_photo_composite_inkset,
  &four_color_standard_inkset,
  &three_color_composite_inkset,
  &one_color_extended_inkset,
  &three_color_extended_inkset,
  &four_color_extended_inkset,
  &f360_five_color_extended_inkset,
  &f360_six_color_extended_inkset,
  &f360_seven_color_extended_inkset,
};

DECLARE_INKLIST(f360_photo7_japan, f360_photo7_japan,
		N_("EPSON Standard Inks"), standard, esp960);

static const escp2_inkname_t *const f360_ultra_photo7_ink_types[] =
{
  &f360_ultra_photo_seven_color_enhanced_inkset,
  &f360_ultra_photo_six_color_photo_inkset,
  &f360_ultra_photo_five_color_photo_composite_inkset,
  &four_color_standard_inkset,
  &three_color_composite_inkset,
  &f360_ultra_photo_two_color_grayscale_inkset,
  &one_color_extended_inkset,
  &f360_ultra_photo_two_color_extended_inkset,
  &three_color_extended_inkset,
  &four_color_extended_inkset,
  &f360_ultra_photo_five_color_extended_inkset,
  &f360_ultra_photo_six_color_extended_inkset,
  &f360_ultra_photo_seven_color_extended_inkset,
};

DECLARE_INKLIST(f360_ultra_photo7, f360_ultra_photo7,
		N_("UltraChrome Photo Black"), standard, ultrachrome_photo);

static const escp2_inkname_t *const ultra_photo7_ink_types[] =
{
  &ultra_photo_seven_color_enhanced_inkset,
  &ultra_photo_six_color_photo_inkset,
  &ultra_photo_five_color_photo_composite_inkset,
  &four_color_standard_inkset,
  &three_color_composite_inkset,
  &ultra_photo_two_color_grayscale_inkset,
  &one_color_extended_inkset,
  &ultra_photo_two_color_extended_inkset,
  &three_color_extended_inkset,
  &four_color_extended_inkset,
  &ultra_photo_five_color_extended_inkset,
  &ultra_photo_six_color_extended_inkset,
  &ultra_photo_seven_color_extended_inkset,
};

DECLARE_INKLIST(ultra_photo7, ultra_photo7,
		N_("UltraChrome Photo Black"), standard, ultrachrome_photo);

static const inklist_t *cmy_group[] =
{
  &cmy_inklist
};

static const inklist_t *standard_group[] =
{
  &standard_inklist,
  &quadtone_inklist
};

static const inklist_t *c80_group[] =
{
  &c80_inklist,
  &c80_quadtone_inklist
};

static const inklist_t *x80_group[] =
{
  &x80_inklist
};

static const inklist_t *photo_gen1_group[] =
{
  &gen1_inklist,
  &quadtone_inklist
};

static const inklist_t *photo_gen2_group[] =
{
  &gen2_inklist,
  &quadtone_inklist
};

static const inklist_t *photo_pigment_group[] =
{
  &pigment_inklist
};

static const inklist_t *f360_photo_group[] =
{
  &f360_photo_inklist
};

static const inklist_t *f360_photo7_japan_group[] =
{
  &f360_photo7_japan_inklist
};

static const inklist_t *f360_ultrachrome_group[] =
{
  &f360_ultra_photo7_inklist
};

static const inklist_t *ultrachrome_group[] =
{
  &ultra_photo7_inklist
};

#define DECLARE_INKGROUP(name)			\
const inkgroup_t stpi_escp2_##name##_inkgroup =	\
{						\
  #name,					\
  name##_group,					\
  sizeof(name##_group) / sizeof(inklist_t *),	\
}

DECLARE_INKGROUP(cmy);
DECLARE_INKGROUP(standard);
DECLARE_INKGROUP(c80);
DECLARE_INKGROUP(x80);
DECLARE_INKGROUP(photo_gen1);
DECLARE_INKGROUP(photo_gen2);
DECLARE_INKGROUP(photo_pigment);
DECLARE_INKGROUP(ultrachrome);
DECLARE_INKGROUP(f360_photo);
DECLARE_INKGROUP(f360_photo7_japan);
DECLARE_INKGROUP(f360_ultrachrome);
