/*
 * "$Id: print-pcl.c,v 1.83 2003/01/18 21:01:08 rlk Exp $"
 *
 *   Print plug-in HP PCL driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and
 *      Dave Hill (dave@minnie.demon.co.uk)
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
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include <gimp-print/gimp-print-intl-internal.h>
#include "gimp-print-internal.h"
#include "module.h"
#include <stdio.h>
#include <string.h>

/* #define DEBUG */
/* #define PCL_DEBUG_DISABLE_COMPRESSION */
/* #define PCL_DEBUG_DISABLE_BLANKLINE_REMOVAL */

/*
 * Local functions...
 */
static void	pcl_mode0(const stp_vars_t, unsigned char *, unsigned char *,
			  int, int);
static void	pcl_mode2(const stp_vars_t, unsigned char *, unsigned char *,
			  int, int);

/*
 * Generic define for a name/value set
 */

typedef struct
{
  const char	*pcl_name;
  const char	*pcl_text;
  int		pcl_code;
  int		p0;
  int		p1;
} pcl_t;

/*
 * Media size to PCL media size code table
 *
 * Note, you can force the list of papersizes given in the GUI to be only those
 * supported by defining PCL_NO_CUSTOM_PAPERSIZES
 */

/* #define PCL_NO_CUSTOM_PAPERSIZES */

#define PCL_PAPERSIZE_EXECUTIVE		1
#define PCL_PAPERSIZE_LETTER		2
#define PCL_PAPERSIZE_LEGAL		3
#define PCL_PAPERSIZE_TABLOID		6	/* "Ledger" */
#define PCL_PAPERSIZE_STATEMENT		15	/* Called "Manual" in print-util */
#define PCL_PAPERSIZE_SUPER_B		16	/* Called "13x19" in print-util */
#define PCL_PAPERSIZE_A5		25
#define PCL_PAPERSIZE_A4		26
#define PCL_PAPERSIZE_A3		27
#define PCL_PAPERSIZE_JIS_B5		45
#define PCL_PAPERSIZE_JIS_B4		46
#define PCL_PAPERSIZE_HAGAKI_CARD	71
#define PCL_PAPERSIZE_OUFUKU_CARD	72
#define PCL_PAPERSIZE_A6_CARD		73
#define PCL_PAPERSIZE_4x6		74
#define PCL_PAPERSIZE_5x8		75
#define PCL_PAPERSIZE_3x5		78
#define PCL_PAPERSIZE_MONARCH_ENV	80
#define PCL_PAPERSIZE_COMMERCIAL10_ENV	81
#define PCL_PAPERSIZE_DL_ENV		90
#define PCL_PAPERSIZE_C5_ENV		91
#define PCL_PAPERSIZE_C6_ENV		92
#define PCL_PAPERSIZE_CUSTOM		101	/* Custom size */
#define PCL_PAPERSIZE_INVITATION_ENV	109
#define PCL_PAPERSIZE_JAPANESE_3_ENV	110
#define PCL_PAPERSIZE_JAPANESE_4_ENV	111
#define PCL_PAPERSIZE_KAKU_ENV		113
#define PCL_PAPERSIZE_HP_CARD		114	/* HP Greeting card!?? */

/*
 * This data comes from the HP documentation "Deskjet 1220C and 1120C
 * PCL reference guide 2.0, Nov 1999". NOTE: The first name *must* match
 * those in print-util.c for the lookups to work properly!
 * The long names are not used so they have been removed, the ones in
 * print-util.c are used in the interface.
 */

static const pcl_t pcl_media_sizes[] =
{
    { "Executive", "notused", PCL_PAPERSIZE_EXECUTIVE},			/* US Exec (7.25 x 10.5 in) */
    { "Letter", "notused", PCL_PAPERSIZE_LETTER},			/* US Letter (8.5 x 11 in) */
    { "Legal", "notused", PCL_PAPERSIZE_LEGAL},				/* US Legal (8.5 x 14 in) */
    { "Tabloid", "notused", PCL_PAPERSIZE_TABLOID},			/* US Tabloid (11 x 17 in) */
    { "Statement", "notused", PCL_PAPERSIZE_STATEMENT},			/* US Manual/Statement (5.5 x 8.5 in) */
    { "SuperB", "notused", PCL_PAPERSIZE_SUPER_B},			/* US 13x19/Super B (13 x 19 in) */
    { "A5", "notused", PCL_PAPERSIZE_A5},				/* ISO/JIS A5 (148 x 210 mm) */
    { "A4", "notused", PCL_PAPERSIZE_A4},				/* ISO/JIS A4 (210 x 297 mm) */
    { "A3", "notused", PCL_PAPERSIZE_A3},				/* ISO/JIS A3 (297 x 420 mm) */
    { "B5", "notused", PCL_PAPERSIZE_JIS_B5},				/* JIS B5 (182 x 257 mm). */
    { "B4", "notused", PCL_PAPERSIZE_JIS_B4},				/* JIS B4 (257 x 364 mm). */
    { "w283h420", "notused", PCL_PAPERSIZE_HAGAKI_CARD},		/* Japanese Hagaki Card (100 x 148 mm) */
    { "w420h567", "notused", PCL_PAPERSIZE_OUFUKU_CARD},		/* Japanese Oufuku Card (148 x 200 mm) */
    { "A6", "notused", PCL_PAPERSIZE_A6_CARD},				/* ISO/JIS A6 card */
    { "w288h432", "notused", PCL_PAPERSIZE_4x6},			/* US Index card (4 x 6 in) */
    { "w360h576", "notused", PCL_PAPERSIZE_5x8},			/* US Index card (5 x 8 in) */
    { "w216h360", "notused", PCL_PAPERSIZE_3x5},			/* US Index card (3 x 5 in) */
    { "Monarch", "notused", PCL_PAPERSIZE_MONARCH_ENV},			/* Monarch Envelope (3 7/8 x 7 1/2 in) */
    { "COM10", "notused", PCL_PAPERSIZE_COMMERCIAL10_ENV},		/* US Commercial 10 Envelope (4.125 x 9.5 in) Portrait */
    { "DL", "notused", PCL_PAPERSIZE_DL_ENV},				/* DL envelope (110 x 220 mm) Portrait */
    { "C5", "notused", PCL_PAPERSIZE_C5_ENV},				/* C5 envelope (162 x 229 mm) */
    { "C6", "notused", PCL_PAPERSIZE_C6_ENV},				/* C6 envelope (114 x 162 mm) */
    { "w315h414", "notused", PCL_PAPERSIZE_INVITATION_ENV},		/* US A2 Invitation envelope (4 3/8 x 5 3/4 in) */
    { "w340h666", "notused", PCL_PAPERSIZE_JAPANESE_3_ENV},		/* Japanese Long Envelope #3 (120 x 235 mm) */
    { "w255h581", "notused", PCL_PAPERSIZE_JAPANESE_4_ENV},		/* Japanese Long Envelope #4 (90 x 205 mm) */
    { "w680h941", "notused", PCL_PAPERSIZE_KAKU_ENV},			/* Japanese Kaku Envelope (240 x 332.1 mm) */
/**** MRS: this size not supported by print-util funcs! ****/
    { "w612h792", "notused", PCL_PAPERSIZE_HP_CARD}, 			/* Hp greeting card */
};
#define NUM_PRINTER_PAPER_SIZES	(sizeof(pcl_media_sizes) / sizeof(pcl_t))

/*
 * Media type to code table
 */

#define PCL_PAPERTYPE_PLAIN	0
#define PCL_PAPERTYPE_BOND	1
#define PCL_PAPERTYPE_PREMIUM	2
#define PCL_PAPERTYPE_GLOSSY	3	/* or photo */
#define PCL_PAPERTYPE_TRANS	4
#define PCL_PAPERTYPE_QPHOTO	5	/* Quick dry photo (2000 only) */
#define PCL_PAPERTYPE_QTRANS	6	/* Quick dry transparency (2000 only) */

static const pcl_t pcl_media_types[] =
{
    { "Plain", N_ ("Plain"), PCL_PAPERTYPE_PLAIN},
    { "Bond", N_ ("Bond"), PCL_PAPERTYPE_BOND},
    { "Premium", N_ ("Premium"), PCL_PAPERTYPE_PREMIUM},
    { "Glossy", N_ ("Glossy Photo"), PCL_PAPERTYPE_GLOSSY},
    { "Transparency", N_ ("Transparency"), PCL_PAPERTYPE_TRANS},
    { "GlossyQD", N_ ("Quick-dry Photo"), PCL_PAPERTYPE_QPHOTO},
    { "TransparencyQD", N_ ("Quick-dry Transparency"), PCL_PAPERTYPE_QTRANS},
};
#define NUM_PRINTER_PAPER_TYPES	(sizeof(pcl_media_types) / sizeof(pcl_t))

/*
 * Media feed to code table. There are different names for the same code,
 * so we encode them by adding "lumps" of "PAPERSOURCE_MOD".
 * This is removed later to get back to the main codes.
 */

#define PAPERSOURCE_MOD			16

#define PCL_PAPERSOURCE_STANDARD	0	/* Don't output code */
#define PCL_PAPERSOURCE_MANUAL		2
#define PCL_PAPERSOURCE_ENVELOPE	3	/* Not used */

/* LaserJet types */
#define PCL_PAPERSOURCE_LJ_TRAY2	1
#define PCL_PAPERSOURCE_LJ_TRAY3	4
#define PCL_PAPERSOURCE_LJ_TRAY4	5
#define PCL_PAPERSOURCE_LJ_TRAY1	8

/* Deskjet 340 types */
#define PCL_PAPERSOURCE_340_PCSF	1 + PAPERSOURCE_MOD
						/* Portable sheet feeder for 340 */
#define PCL_PAPERSOURCE_340_DCSF	4 + PAPERSOURCE_MOD
						/* Desktop sheet feeder for 340 */

/* Other Deskjet types */
#define PCL_PAPERSOURCE_DJ_TRAY		1 + PAPERSOURCE_MOD + PAPERSOURCE_MOD
#define PCL_PAPERSOURCE_DJ_TRAY2	4 + PAPERSOURCE_MOD + PAPERSOURCE_MOD
						/* Tray 2 for 2500 */
#define PCL_PAPERSOURCE_DJ_OPTIONAL	5 + PAPERSOURCE_MOD + PAPERSOURCE_MOD
						/* Optional source for 2500 */
#define PCL_PAPERSOURCE_DJ_AUTO		7 + PAPERSOURCE_MOD + PAPERSOURCE_MOD
						/* Autoselect for 2500 */

static const pcl_t pcl_media_sources[] =
{
    { "Standard", N_ ("Standard"), PCL_PAPERSOURCE_STANDARD},
    { "Manual", N_ ("Manual"), PCL_PAPERSOURCE_MANUAL},
/*  {"Envelope", PCL_PAPERSOURCE_ENVELOPE}, */
    { "MultiPurpose", N_ ("Tray 1"), PCL_PAPERSOURCE_LJ_TRAY1},
    { "Upper", N_ ("Tray 2"), PCL_PAPERSOURCE_LJ_TRAY2},
    { "Lower", N_ ("Tray 3"), PCL_PAPERSOURCE_LJ_TRAY3},
    { "LargeCapacity", N_ ("Tray 4"), PCL_PAPERSOURCE_LJ_TRAY4},
    { "Portable", N_ ("Portable Sheet Feeder"), PCL_PAPERSOURCE_340_PCSF},
    { "Desktop", N_ ("Desktop Sheet Feeder"), PCL_PAPERSOURCE_340_DCSF},
    { "Tray", N_ ("Tray"), PCL_PAPERSOURCE_DJ_TRAY},
    { "Tray2", N_ ("Tray 2"), PCL_PAPERSOURCE_DJ_TRAY2},
    { "Optional", N_ ("Optional Source"), PCL_PAPERSOURCE_DJ_OPTIONAL},
    { "Auto", N_ ("Autoselect"), PCL_PAPERSOURCE_DJ_AUTO},
};
#define NUM_PRINTER_PAPER_SOURCES	(sizeof(pcl_media_sources) / sizeof(pcl_t))

#define PCL_RES_150_150		1
#define PCL_RES_300_300		2
#define PCL_RES_600_300		4	/* DJ 600 series */
#define PCL_RES_600_600_MONO	8	/* DJ 600/800/1100/2000 b/w only */
#define PCL_RES_600_600		16	/* DJ 9xx/1220C/PhotoSmart */
#define PCL_RES_1200_600	32	/* DJ 9xx/1220C/PhotoSmart */
#define PCL_RES_2400_600	64	/* DJ 9xx/1220C/PhotoSmart */

static const pcl_t pcl_resolutions[] =
{
    { "150dpi", N_("150x150 DPI"), PCL_RES_150_150, 150, 150},
    { "300dpi", N_("300x300 DPI"), PCL_RES_300_300, 300, 300},
    { "600x300dpi", N_("600x300 DPI"), PCL_RES_600_300, 600, 300},
    { "600mono", N_("600x600 DPI monochrome"), PCL_RES_600_600_MONO, 600, 600},
    { "600dpi", N_("600x600 DPI"), PCL_RES_600_600, 600, 600},
    { "1200x600dpi", N_("1200x600 DPI"), PCL_RES_1200_600, 1200, 600},
    { "2400x600dpi", N_("2400x600 DPI"), PCL_RES_2400_600, 2400, 600},
};
#define NUM_RESOLUTIONS		(sizeof(pcl_resolutions) / sizeof (pcl_t))

static void
pcl_describe_resolution(const stp_vars_t v, int *x, int *y)
{
  int i;
  const char *resolution = stp_get_string_parameter(v, "Resolution");
  if (resolution)
    {
      for (i = 0; i < NUM_RESOLUTIONS; i++)
	{
	  if (!strcmp(resolution, pcl_resolutions[i].pcl_name))
	    {
	      *x = pcl_resolutions[i].p0;
	      *y = pcl_resolutions[i].p1;
	      return;
	    }
	}
    }
  *x = -1;
  *y = -1;
}

typedef struct {
  int top_margin;
  int bottom_margin;
  int left_margin;
  int right_margin;
} margins_t;

/*
 * Printer capability data
 */

typedef struct {
  int model;
  int custom_max_width;
  int custom_max_height;
  int custom_min_width;
  int custom_min_height;
  int resolutions;
  margins_t normal_margins;
  margins_t a4_margins;
  int color_type;		/* 2 print head or one, 2 level or 4 */
  int stp_printer_type;		/* Deskjet/Laserjet and quirks */
/* The paper size, paper type and paper source codes cannot be combined */
  const int paper_sizes[NUM_PRINTER_PAPER_SIZES + 1];
				/* Paper sizes */
  const int paper_types[NUM_PRINTER_PAPER_TYPES + 1];
				/* Paper types */
  const int paper_sources[NUM_PRINTER_PAPER_SOURCES + 1];
				/* Paper sources */
  } pcl_cap_t;

#define PCL_COLOR_NONE		0
#define PCL_COLOR_CMY		1	/* One print head */
#define PCL_COLOR_CMYK		2	/* Two print heads */
#define PCL_COLOR_CMYK4		4	/* CRet printing */
#define PCL_COLOR_CMYKcm	8	/* CMY + Photo Cart */
#define PCL_COLOR_CMYK4b	16	/* CRet for HP840c */

#define PCL_PRINTER_LJ		1
#define PCL_PRINTER_DJ		2
#define PCL_PRINTER_NEW_ERG	4	/* use "\033*rC" to end raster graphics,
					   instead of "\033*rB" */
#define PCL_PRINTER_TIFF	8	/* Use TIFF compression */
#define PCL_PRINTER_MEDIATYPE	16	/* Use media type & print quality */
#define PCL_PRINTER_CUSTOM_SIZE	32	/* Custom sizes supported */
#define PCL_PRINTER_BLANKLINE	64	/* Blank line removal supported */

/*
 * FIXME - the 520 shouldn't be lumped in with the 500 as it supports
 * more paper sizes.
 *
 * The following models use depletion, raster quality and shingling:-
 * 500, 500c, 510, 520, 550c, 560c.
 * The rest use Media Type and Print Quality.
 *
 * This data comes from the HP documentation "Deskjet 1220C and 1120C
 * PCL reference guide 2.0, Nov 1999".
 */

static const pcl_cap_t pcl_model_capabilities[] =
{
  /* Default/unknown printer - assume laserjet */
  { 0,
    17 * 72 / 2, 14 * 72,		/* Max paper size */
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,	/* Resolutions */
    {12, 12, 18, 18},			/* non-A4 Margins */
    {12, 12, 10, 10},			/* A4 Margins */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      0,
    },
    { -1,			/* No selectable paper types */
    },
    { -1,			/* No selectable paper sources */
    },
  },
  /* DesignJet 230/430 (monochrome ) */
  { 10230,
    36 * 72, 150 * 12 * 72,		/* 150ft in roll mode, 64" in sheet */
    830 * 72 / 100, 583 * 72 / 100,	/* 8.3" wide min in sheet mode */
    PCL_RES_300_300 | PCL_RES_600_600,
    {49, 49, 15, 15},
    {49, 49, 15, 15},
    PCL_COLOR_NONE,
    PCL_PRINTER_DJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE | PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_NEW_ERG,
    {
      PCL_PAPERSIZE_LETTER,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      -1,
    },
  },
  /* DesignJet 250C/450C/455CA/488CA */
  /* The "CA" versions have a "software RIP" but are the same hardware */
  { 10250,
    36 * 72, 150 * 12 * 72,		/* 150ft in roll mode, 64" in sheet */
    830 * 72 / 100, 583 * 72 / 100,	/* 8.3" wide min in sheet mode */
    PCL_RES_300_300 | PCL_RES_600_600_MONO,
    {49, 49, 15, 15},
    {49, 49, 15, 15},
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE | PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_NEW_ERG,
    {
      PCL_PAPERSIZE_LETTER,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      -1,
    },
  },
  /* DesignJet 700 (monochrome) */
  { 10700,
    36 * 72, 150 * 12 * 72,		/* 150ft in roll mode, 64" in sheet */
    830 * 72 / 100, 583 * 72 / 100,	/* 8.3" wide min in sheet mode */
    PCL_RES_300_300 | PCL_RES_600_600,
    {30, 30, 15, 15},		/* These margins are for sheet mode FIX */
    {30, 30, 15, 15},
    PCL_COLOR_NONE,
    PCL_PRINTER_DJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE | PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_NEW_ERG,
    {
      PCL_PAPERSIZE_LETTER,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      -1,
    },
  },
  /* DesignJet 750C */
  { 10750,
    36 * 72, 150 * 12 * 72,		/* 150ft in roll mode, 64" in sheet */
    830 * 72 / 100, 583 * 72 / 100,	/* 8.3" wide min in sheet mode */
    PCL_RES_300_300 | PCL_RES_600_600_MONO,
    {30, 30, 15, 15},	/* These margins are for roll mode FIX */
    {30, 30, 15, 15},
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE | PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_NEW_ERG,
    {
      PCL_PAPERSIZE_LETTER,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      -1,
    },
  },
  /* DesignJet 2500C/3500C (44" wide) */
  { 12500,	/* Deskjet 2500 already has "2500" */
    44 * 72, 150 * 12 * 72,		/* 150ft in roll mode, 64" in sheet */
    830 * 72 / 100, 583 * 72 / 100,	/* 8.3" wide min in sheet mode */
    PCL_RES_300_300 | PCL_RES_600_600_MONO,
    {49, 49, 15, 15},		/* Check/Fix */
    {49, 49, 15, 15},
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE | PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_NEW_ERG,
    {
      PCL_PAPERSIZE_LETTER,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      -1,
    },
  },
  /* Deskjet 340 */
  { 340,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {7, 41, 18, 18},
    {7, 41, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_340_PCSF,
      PCL_PAPERSOURCE_340_DCSF,
      -1,
    },
  },
  /* Deskjet 400 */
  { 400,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {7, 41, 18, 18},
    {7, 41, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_JIS_B5,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    { -1,			/* No selectable paper sources */
    },
  },
  /* Deskjet 500, 520. Lexmark 4076 */
  { 500,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {7, 41, 18, 18},
    {7, 41, 10, 10},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_DJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    {
/*    PCL_PAPERSIZE_EXECUTIVE,	The 500 doesn't support this, the 520 does */
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
/*    PCL_PAPERSIZE_DL_ENV,	The 500 doesn't support this, the 520 does */
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 500C */
  { 501,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {7, 33, 18, 18},
    {7, 33, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 540C */
  { 540,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {7, 33, 18, 18},
    {7, 33, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_JIS_B5,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 550C, 560C */
  { 550,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {3, 33, 18, 18},
    {5, 33, 10, 10},
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
/* The 550/560 support COM10 and DL envelope, but the control codes
   are negative, indicating landscape mode. This needs thinking about! */
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 600/600C */
  { 600,
    17 * 72 / 2, 14 * 72,
    5 * 72, 583 * 72 / 100,		/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_300 | PCL_RES_600_600_MONO,
    {0, 33, 18, 18},
    {0, 33, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    { -1,			/* No selectable paper sources */
    },
  },
  /* Deskjet 6xx series */
  { 601,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_300 | PCL_RES_600_600_MONO,
    {0, 33, 18, 18},
    {0, 33, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      -1,
    },
  },
  /* Deskjet 69x series (Photo Capable) */
  { 690,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_300 | PCL_RES_600_600,
    {0, 33, 18, 18},
    {0, 33, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMYK | PCL_COLOR_CMYKcm,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      -1,
    },
  },
  /* Deskjet 850/855/870/890 (C-RET) */
  { 800,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600_MONO,
    {3, 33, 18, 18},
    {5, 33, 10, 10},
    PCL_COLOR_CMYK | PCL_COLOR_CMYK4,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      -1,
    },
  },
  /* Deskjet 810C, 812C, 840C, 842C, 845C, 895C (C-RET) */
  { 840,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_300 | PCL_RES_600_600,
    {0, 33, 18, 18},
    {0, 33, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMYK | PCL_COLOR_CMYK4b,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      -1,
    },
  },
  /* Deskjet 900 series, 1220C, PhotoSmart P1000/P1100 */
  { 900,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600 /* | PCL_RES_1200_600 | PCL_RES_2400_600 */,
    {3, 33, 18, 18},
    {5, 33, 10, 10},	/* Oliver Vecernik */
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    { -1,			/* No selectable paper sources */
    },
  },
  /* Deskjet 1220C (or other large format 900) */
  { 901,
    13 * 72, 19 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600 /* | PCL_RES_1200_600 | PCL_RES_2400_600 */,
    {3, 33, 18, 18},
    {5, 33, 10, 10},
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_TABLOID,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_SUPER_B,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_A3,
      PCL_PAPERSIZE_JIS_B5,
      PCL_PAPERSIZE_JIS_B4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_OUFUKU_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_3x5,
      PCL_PAPERSIZE_HP_CARD,
      PCL_PAPERSIZE_MONARCH_ENV,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C5_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      PCL_PAPERSIZE_JAPANESE_3_ENV,
      PCL_PAPERSIZE_JAPANESE_4_ENV,
      PCL_PAPERSIZE_KAKU_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    { -1,			/* No selectable paper sources */
    },
  },
  /* Deskjet 1100C, 1120C */
  { 1100,
    13 * 72, 19 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600_MONO,
    {3, 33, 18, 18},
    {5, 33, 10, 10},
    PCL_COLOR_CMYK | PCL_COLOR_CMYK4,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_TABLOID,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_SUPER_B,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_A3,
      PCL_PAPERSIZE_JIS_B5,
      PCL_PAPERSIZE_JIS_B4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      PCL_PAPERSIZE_JAPANESE_3_ENV,
      PCL_PAPERSIZE_JAPANESE_4_ENV,
      PCL_PAPERSIZE_KAKU_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 1200C */
  { 1200,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMY,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
/* This printer is not mentioned in the Comparison tables,
   so I'll just pick some likely sizes... */
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 1600C */
  { 1600,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
/* This printer is not mentioned in the Comparison tables,
   so I'll just pick some likely sizes... */
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 2000 */
  { 2000,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    {0, 35, 18, 18},			/* Michel Goraczko */
    {0, 35, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_3x5,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C6_ENV,
      PCL_PAPERSIZE_INVITATION_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      PCL_PAPERTYPE_QPHOTO,
      PCL_PAPERTYPE_QTRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_TRAY,
      -1,
    },
  },
  /* Deskjet 2500 */
  { 2500,
    13 * 72, 19 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_CMYK,
    PCL_PRINTER_DJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_MEDIATYPE |
      PCL_PRINTER_CUSTOM_SIZE | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_TABLOID,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_A3,
      PCL_PAPERSIZE_JIS_B5,
      PCL_PAPERSIZE_JIS_B4,
      PCL_PAPERSIZE_HAGAKI_CARD,
      PCL_PAPERSIZE_A6_CARD,
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      -1,
    },
    {
      PCL_PAPERTYPE_PLAIN,
      PCL_PAPERTYPE_BOND,
      PCL_PAPERTYPE_PREMIUM,
      PCL_PAPERTYPE_GLOSSY,
      PCL_PAPERTYPE_TRANS,
      PCL_PAPERTYPE_QPHOTO,
      PCL_PAPERTYPE_QTRANS,
      -1,
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_DJ_AUTO,
      PCL_PAPERSOURCE_DJ_TRAY,
      PCL_PAPERSOURCE_DJ_TRAY2,
      PCL_PAPERSOURCE_DJ_OPTIONAL,
      -1,
    },
  },
  /* LaserJet II series */
  { 2,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_MONARCH_ENV,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C5_ENV,
      PCL_PAPERSIZE_C6_ENV,
      -1,
    },
    { -1,			/* No selectable paper types */
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_LJ_TRAY1,
      PCL_PAPERSOURCE_LJ_TRAY2,
      PCL_PAPERSOURCE_LJ_TRAY3,
      PCL_PAPERSOURCE_LJ_TRAY4,
      -1,
    },
  },
  /* LaserJet III series */
  { 3,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_MONARCH_ENV,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C5_ENV,
      PCL_PAPERSIZE_C6_ENV,
      -1,
    },
    { -1,			/* No selectable paper types */
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_LJ_TRAY1,
      PCL_PAPERSOURCE_LJ_TRAY2,
      PCL_PAPERSOURCE_LJ_TRAY3,
      PCL_PAPERSOURCE_LJ_TRAY4,
      -1,
    },
  },
  /* LaserJet 4L */
  { 4,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_MONARCH_ENV,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C5_ENV,
      PCL_PAPERSIZE_C6_ENV,
      -1,
    },
    { -1,			/* No selectable paper types */
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_LJ_TRAY1,
      PCL_PAPERSOURCE_LJ_TRAY2,
      PCL_PAPERSOURCE_LJ_TRAY3,
      PCL_PAPERSOURCE_LJ_TRAY4,
      -1,
    },
  },
  /* LaserJet 4V, 4Si */
  { 5,
    13 * 72, 19 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_TABLOID,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_A3,
      PCL_PAPERSIZE_JIS_B5,
      PCL_PAPERSIZE_JIS_B4,		/* Guess */
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_MONARCH_ENV,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C5_ENV,
      PCL_PAPERSIZE_C6_ENV,
      -1,
    },
    { -1,			/* No selectable paper types */
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_LJ_TRAY1,
      PCL_PAPERSOURCE_LJ_TRAY2,
      PCL_PAPERSOURCE_LJ_TRAY3,
      PCL_PAPERSOURCE_LJ_TRAY4,
      -1,
    },
  },
  /* LaserJet 4 series (except as above), 5 series, 6 series */
  { 6,
    17 * 72 / 2, 14 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_MONARCH_ENV,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C5_ENV,
      PCL_PAPERSIZE_C6_ENV,
      -1,
    },
    { -1,			/* No selectable paper types */
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_LJ_TRAY1,
      PCL_PAPERSOURCE_LJ_TRAY2,
      PCL_PAPERSOURCE_LJ_TRAY3,
      PCL_PAPERSOURCE_LJ_TRAY4,
      -1,
    },
  },
  /* LaserJet 5Si */
  { 7,
    13 * 72, 19 * 72,
    1, 1,				/* Min paper size */
    PCL_RES_150_150 | PCL_RES_300_300 | PCL_RES_600_600,
    {12, 12, 18, 18},
    {12, 12, 10, 10},	/* Check/Fix */
    PCL_COLOR_NONE,
    PCL_PRINTER_LJ | PCL_PRINTER_NEW_ERG | PCL_PRINTER_TIFF | PCL_PRINTER_BLANKLINE,
    {
      PCL_PAPERSIZE_EXECUTIVE,
      PCL_PAPERSIZE_STATEMENT,
      PCL_PAPERSIZE_LETTER,
      PCL_PAPERSIZE_LEGAL,
      PCL_PAPERSIZE_TABLOID,
      PCL_PAPERSIZE_A5,
      PCL_PAPERSIZE_A4,
      PCL_PAPERSIZE_A3,
      PCL_PAPERSIZE_JIS_B5,
      PCL_PAPERSIZE_JIS_B4,		/* Guess */
      PCL_PAPERSIZE_4x6,
      PCL_PAPERSIZE_5x8,
      PCL_PAPERSIZE_MONARCH_ENV,
      PCL_PAPERSIZE_COMMERCIAL10_ENV,
      PCL_PAPERSIZE_DL_ENV,
      PCL_PAPERSIZE_C5_ENV,
      PCL_PAPERSIZE_C6_ENV,
      -1,
    },
    { -1,			/* No selectable paper types */
    },
    {
      PCL_PAPERSOURCE_STANDARD,
      PCL_PAPERSOURCE_MANUAL,
      PCL_PAPERSOURCE_LJ_TRAY1,
      PCL_PAPERSOURCE_LJ_TRAY2,
      PCL_PAPERSOURCE_LJ_TRAY3,
      PCL_PAPERSOURCE_LJ_TRAY4,
      -1,
    },
  },
};


static const char standard_sat_adjustment[] =
"STP_CURVE;Wrap ;Linear ; 48;0;0.0;4.0:"
"1.00;1.10;1.20;1.30;1.40;1.50;1.60;1.70;"  /* C */
"1.80;1.90;1.90;1.90;1.70;1.50;1.30;1.10;"  /* B */
"1.00;1.00;1.00;1.00;1.00;1.00;1.00;1.00;"  /* M */
"1.00;1.00;1.00;1.00;1.00;1.00;1.00;1.00;"  /* R */
"1.00;1.00;1.00;1.10;1.20;1.30;1.40;1.50;"  /* Y */
"1.50;1.40;1.30;1.20;1.10;1.00;1.00;1.00;"; /* G */

static const char standard_lum_adjustment[] =
"STP_CURVE;Wrap ;Linear ; 48;0;0.0;4.0:"
"0.50;0.60;0.70;0.80;0.90;0.86;0.82;0.79;"  /* C */
"0.78;0.80;0.83;0.87;0.90;0.95;1.05;1.15;"  /* B */
"1.30;1.25;1.20;1.15;1.12;1.09;1.06;1.03;"  /* M */
"1.00;1.00;1.00;1.00;1.00;1.00;1.00;1.00;"  /* R */
"1.00;0.90;0.80;0.70;0.65;0.60;0.55;0.52;"  /* Y */
"0.48;0.47;0.47;0.49;0.49;0.49;0.52;0.51;"; /* G */

static const char standard_hue_adjustment[] =
"STP_CURVE;Wrap ;Linear ; 48;0;-6.0;6.0:"
"0.00;0.05;0.04;0.01;-.03;-.10;-.18;-.26;"  /* C */
"-.35;-.43;-.40;-.32;-.25;-.18;-.10;-.07;"  /* B */
"0.00;-.04;-.09;-.13;-.18;-.23;-.27;-.31;"  /* M */
"-.35;-.38;-.30;-.23;-.15;-.08;0.00;-.02;"  /* R */
"0.00;0.08;0.10;0.08;0.05;0.03;-.03;-.12;"  /* Y */
"-.20;0.17;-.20;-.17;-.15;-.12;-.10;-.08;"; /* G */

static const stp_parameter_t the_parameters[] =
{
  {
    "PageSize", N_("Page Size"),
    N_("Size of the paper being printed to"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_PAGE_SIZE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1
  },
  {
    "MediaType", N_("Media Type"),
    N_("Type of media (plain paper, photo paper, etc.)"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1
  },
  {
    "InputSlot", N_("Media Source"),
    N_("Source (input slot) of the media"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1
  },
  {
    "InkType", N_("Ink Type"),
    N_("Type of ink in the printer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1
  },
  {
    "Resolution", N_("Resolution"),
    N_("Resolution and quality of the print"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1
  },
};

static int the_parameter_count =
sizeof(the_parameters) / sizeof(const stp_parameter_t);

/*
 * Convert a name into it's option value
 */

static int pcl_string_to_val(const char *string,		/* I: String */
                           const pcl_t *options,	/* I: Options */
			   int num_options)		/* I: Num options */
{

  int i;
  int code = -1;

 /*
  * Look up the string in the table and convert to the code.
  */

  for (i=0; i<num_options; i++) {
    if (!strcmp(string, options[i].pcl_name)) {
       code=options[i].pcl_code;
       break;
       }
  }

  stp_deprintf(STP_DBG_PCL, "String: %s, Code: %d\n", string, code);

  return(code);
}

/*
 * Convert a value into it's option name
 */

static const char * pcl_val_to_string(int code,			/* I: Code */
                           const pcl_t *options,	/* I: Options */
			   int num_options)		/* I: Num options */
{

  int i;
  const char *string = NULL;

 /*
  * Look up the code in the table and convert to the string.
  */

  for (i=0; i<num_options; i++) {
    if (code == options[i].pcl_code) {
       string=options[i].pcl_name;
       break;
       }
  }

  stp_deprintf(STP_DBG_PCL, "Code: %d, String: %s\n", code, string);

  return(string);
}

static const char * pcl_val_to_text(int code,			/* I: Code */
                           const pcl_t *options,	/* I: Options */
			   int num_options)		/* I: Num options */
{

  int i;
  const char *string = NULL;

 /*
  * Look up the code in the table and convert to the string.
  */

  for (i=0; i<num_options; i++) {
    if (code == options[i].pcl_code) {
       string=options[i].pcl_text;
       break;
       }
  }

  stp_deprintf(STP_DBG_PCL, "Code: %d, String: %s\n", code, string);

  return(string);
}

static const double dot_sizes[] = { 0.5, 0.832, 1.0 };
static const stp_dither_range_simple_t variable_dither_ranges[] =
{
  { 0.152, 0x1, 1 },
  { 0.255, 0x2, 1 },
  { 0.38,  0x3, 1 },
  { 0.5,   0x1, 0 },
  { 0.67,  0x2, 0 },
  { 1.0,   0x3, 0 }
};

/*
 * pcl_get_model_capabilities() - Return struct of model capabilities
 */

static const pcl_cap_t *				/* O: Capabilities */
pcl_get_model_capabilities(int model)	/* I: Model */
{
  int i;
  int models= sizeof(pcl_model_capabilities) / sizeof(pcl_cap_t);
  for (i=0; i<models; i++) {
    if (pcl_model_capabilities[i].model == model) {
      return &(pcl_model_capabilities[i]);
    }
  }
  stp_erprintf("pcl: model %d not found in capabilities list.\n",model);
  return &(pcl_model_capabilities[0]);
}

/*
 * Convert Media size name into PCL media code for printer
 */

static int pcl_convert_media_size(const char *media_size,	/* I: Media size string */
				  int  model)		/* I: model number */
{

  int i;
  int media_code = 0;
  const pcl_cap_t *caps;

 /*
  * First look up the media size in the table and convert to the code.
  */

  media_code = pcl_string_to_val(media_size, pcl_media_sizes,
                                 sizeof(pcl_media_sizes) / sizeof(pcl_t));

  stp_deprintf(STP_DBG_PCL, "Media Size: %s, Code: %d\n", media_size, media_code);

 /*
  * Now see if the printer supports the code found.
  */

  if (media_code != -1) {
    caps = pcl_get_model_capabilities(model);

    for (i=0; (i<NUM_PRINTER_PAPER_SIZES) && (caps->paper_sizes[i] != -1); i++) {
      if (media_code == caps->paper_sizes[i])
        return(media_code);		/* Is supported */
    }

    stp_deprintf(STP_DBG_PCL, "Media Code %d not supported by printer model %d.\n",
      media_code, model);
    return(-1);				/* Not supported */
  }
  else
    return(-1);				/* Not supported */
}


static stp_param_string_t ink_types[] =
{
  { "CMYK",	N_ ("Color + Black Cartridges") },
  { "Photo",	N_ ("Color + Photo Cartridges") }
};


/*
 * 'pcl_papersize_valid()' - Is the paper size valid for this printer.
 */

static const int
pcl_papersize_valid(const stp_papersize_t pt,
		    int model)
{

  const pcl_cap_t *caps = pcl_get_model_capabilities(model);

#ifdef PCL_NO_CUSTOM_PAPERSIZES
  int use_custom = 0;
#else
  int use_custom = ((caps->stp_printer_type & PCL_PRINTER_CUSTOM_SIZE)
                     == PCL_PRINTER_CUSTOM_SIZE);
#endif

  unsigned int pwidth = stp_papersize_get_width(pt);
  unsigned int pheight = stp_papersize_get_height(pt);

/*
 * This function decides whether a paper size is allowed for the
 * current printer. The DeskJet feed mechanisms set a minimum and
 * maximum size for the paper, BUT some of the directly supported
 * media sizes are less than this minimum (eg card and envelopes)
 * So, we allow supported sizes though, but clamp custom sizes
 * to the min and max sizes.
 */

/*
 * Is it a valid name?
 */

  if (strlen(stp_papersize_get_name(pt)) <= 0)
    return(0);

/*
 * Is it a recognised supported name?
 */

  if (pcl_convert_media_size(stp_papersize_get_name(pt), model) != -1)
    return(1);

/*
 * If we are not allowed to use custom paper sizes, we are done
 */

  if (use_custom == 0)
    return(0);

/*
 * We are allowed custom paper sizes. Check that the size is within
 * limits.
 */

  if (pwidth <= caps->custom_max_width &&
     pheight <= caps->custom_max_height &&
     (pheight >=  caps->custom_min_height || pheight == 0) &&
     (pwidth >= caps->custom_min_width || pwidth == 0))
    return(1);

  return(0);
}

/*
 * 'pcl_parameters()' - Return the parameter values for the given parameter.
 */

static stp_parameter_list_t
pcl_list_parameters(const stp_vars_t v)
{
  stp_parameter_list_t *ret = stp_parameter_list_create();
  int i;
  for (i = 0; i < the_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(the_parameters[i]));
  return ret;
}

static void
pcl_parameters(const stp_vars_t v, const char *name,
	       stp_parameter_t *description)
{
  int		model = stp_get_model(v);
  int		i;
  const pcl_cap_t *caps;
  description->p_type = STP_PARAMETER_TYPE_INVALID;

  if (name == NULL)
    return;

  stp_deprintf(STP_DBG_PCL, "pcl_parameters(): Name = %s\n", name);

  caps = pcl_get_model_capabilities(model);

  stp_deprintf(STP_DBG_PCL, "Printer model = %d\n", model);
  stp_deprintf(STP_DBG_PCL, "PageWidth = %d, PageHeight = %d\n", caps->custom_max_width, caps->custom_max_height);
  stp_deprintf(STP_DBG_PCL, "MinPageWidth = %d, MinPageHeight = %d\n", caps->custom_min_width, caps->custom_min_height);
  stp_deprintf(STP_DBG_PCL, "Normal Margins: top = %d, bottom = %d, left = %d, right = %d\n",
    caps->normal_margins.top_margin, caps->normal_margins.bottom_margin,
    caps->normal_margins.left_margin, caps->normal_margins.right_margin);
  stp_deprintf(STP_DBG_PCL, "A4 Margins: top = %d, bottom = %d, left = %d, right = %d\n",
    caps->a4_margins.top_margin, caps->a4_margins.bottom_margin,
    caps->a4_margins.left_margin, caps->a4_margins.right_margin);
  stp_deprintf(STP_DBG_PCL, "Resolutions: %d\n", caps->resolutions);
  stp_deprintf(STP_DBG_PCL, "ColorType = %d, PrinterType = %d\n", caps->color_type, caps->stp_printer_type);

  for (i = 0; i < the_parameter_count; i++)
    if (strcmp(name, the_parameters[i].name) == 0)
      {
	stp_fill_parameter_settings(description, &(the_parameters[i]));
	break;
      }
  description->deflt.str = NULL;

  if (strcmp(name, "PageSize") == 0)
    {
      int papersizes = stp_known_papersizes();
      description->bounds.str = stp_string_list_allocate();
      for (i = 0; i < papersizes; i++)
	{
	  const stp_papersize_t pt = stp_get_papersize_by_index(i);
	  if (strlen(stp_papersize_get_name(pt)) > 0 &&
	      pcl_papersize_valid(pt, model))
	    stp_string_list_add_param(description->bounds.str,
				      stp_papersize_get_name(pt),
				      stp_papersize_get_text(pt));
	}
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else if (strcmp(name, "MediaType") == 0)
  {
    description->bounds.str = stp_string_list_allocate();
    if (caps->paper_types[0] != -1)
      {
	for (i=0; (i < NUM_PRINTER_PAPER_TYPES) && (caps->paper_types[i] != -1); i++)
	  stp_string_list_add_param(description->bounds.str,
				    pcl_val_to_string(caps->paper_types[i],
						      pcl_media_types,
						      NUM_PRINTER_PAPER_TYPES),
				    pcl_val_to_text(caps->paper_types[i],
						    pcl_media_types,
						    NUM_PRINTER_PAPER_TYPES));
	description->deflt.str =
	  stp_string_list_param(description->bounds.str, 0)->name;
      }
    else
      description->is_active = 0;
  }
  else if (strcmp(name, "InputSlot") == 0)
  {
    description->bounds.str = stp_string_list_allocate();
    if (caps->paper_sources[0] != -1)
      {
	for (i=0; (i < NUM_PRINTER_PAPER_SOURCES) && (caps->paper_sources[i] != -1); i++)
	  stp_string_list_add_param(description->bounds.str,
				    pcl_val_to_string(caps->paper_sources[i],
						      pcl_media_sources,
						      NUM_PRINTER_PAPER_SOURCES),
				    pcl_val_to_text(caps->paper_sources[i],
						    pcl_media_sources,
						    NUM_PRINTER_PAPER_SOURCES));
	description->deflt.str =
	  stp_string_list_param(description->bounds.str, 0)->name;
      }
    else
      description->is_active = 0;
  }
  else if (strcmp(name, "Resolution") == 0)
  {
    description->bounds.str = stp_string_list_allocate();
    description->deflt.str = NULL;
    for (i = 0; i < NUM_RESOLUTIONS; i++)
      if (caps->resolutions & pcl_resolutions[i].pcl_code)
	{
	  if (pcl_resolutions[i].pcl_code >= PCL_RES_300_300 &&
	      description->deflt.str == NULL)
	    description->deflt.str =
	      pcl_val_to_string(pcl_resolutions[i].pcl_code,
				pcl_resolutions, NUM_RESOLUTIONS);
	  stp_string_list_add_param
	    (description->bounds.str,
	     pcl_val_to_string(pcl_resolutions[i].pcl_code,
			       pcl_resolutions, NUM_RESOLUTIONS),
	     pcl_val_to_text(pcl_resolutions[i].pcl_code,
			     pcl_resolutions, NUM_RESOLUTIONS));
	}
    if (description->deflt.str == NULL)
      stp_erprintf("No default resolution set!\n");
  }
  else if (strcmp(name, "InkType") == 0)
  {
    description->bounds.str = stp_string_list_allocate();
    if (caps->color_type & PCL_COLOR_CMYKcm)
    {
      description->deflt.str = ink_types[0].name;
      stp_string_list_add_param(description->bounds.str,
			       ink_types[0].name,_(ink_types[0].text));
      stp_string_list_add_param(description->bounds.str,
			       ink_types[1].name,_(ink_types[1].text));
    }
    else
      description->is_active = 0;
  }
}


/*
 * 'pcl_imageable_area()' - Return the imageable area of the page.
 */

static void
pcl_imageable_area(const stp_vars_t v,     /* I */
                   int  *left,		/* O - Left position in points */
                   int  *right,		/* O - Right position in points */
                   int  *bottom,	/* O - Bottom position in points */
                   int  *top)		/* O - Top position in points */
{
  int	width, height;			/* Size of page */
  const pcl_cap_t *caps;		/* Printer caps */
  int	pcl_media_size;			/* Converted media size */
  const char *media_size = stp_get_string_parameter(v, "PageSize");
  stp_papersize_t pp;

  caps = pcl_get_model_capabilities(stp_get_model(v));

  stp_default_media_size(v, &width, &height);

/* If we are using A4 paper, then the margins are different than any
 * other paper size. This is because HP wanted to have the same printable
 * width for A4 as for letter. Go figure.
 */

  if (!media_size)
    media_size = "";
  if (strlen(media_size) == 0 &&
      ((pp = stp_get_papersize_by_size(stp_get_page_height(v),
				       stp_get_page_width(v))) != NULL))
    media_size = stp_papersize_get_name(pp);

  stp_deprintf(STP_DBG_PCL, "pcl_imageable_area(): media_size: '%s'\n",
		media_size);

  pcl_media_size = pcl_convert_media_size(media_size, stp_get_model(v));

  if (pcl_media_size == PCL_PAPERSIZE_A4)
  {
    *left   = caps->a4_margins.left_margin;
    *right  = width - caps->a4_margins.right_margin;
    *top    = caps->a4_margins.top_margin;
    *bottom = height - caps->a4_margins.bottom_margin;
  }
  else
  {
    *left   = caps->normal_margins.left_margin;
    *right  = width - caps->normal_margins.right_margin;
    *top    = caps->normal_margins.top_margin;
    *bottom = height - caps->normal_margins.bottom_margin;
  }
}

static void
pcl_limit(const stp_vars_t v,  		/* I */
	  int *width,
	  int *height,
	  int *min_width,
	  int *min_height)
{
  const pcl_cap_t *caps= pcl_get_model_capabilities(stp_get_model(v));
  *width =	caps->custom_max_width;
  *height =	caps->custom_max_height;
  *min_width =	caps->custom_min_width;
  *min_height =	caps->custom_min_height;
}

/*
 * 'pcl_print()' - Print an image to an HP printer.
 */

static int
pcl_print(const stp_vars_t v, stp_image_t *image)
{
  int i;
  int		status = 1;
  int		model = stp_get_model(v);
  const char	*resolution = stp_get_string_parameter(v, "Resolution");
  const char	*media_size = stp_get_string_parameter(v, "PageSize");
  const char	*media_type = stp_get_string_parameter(v, "MediaType");
  const char	*media_source = stp_get_string_parameter(v, "InputSlot");
  const char	*ink_type = stp_get_string_parameter(v, "InkType");
  int 		output_type = stp_get_output_type(v);
  int		top = stp_get_top(v);
  int		left = stp_get_left(v);
  int		y;		/* Looping vars */
  int		xdpi, ydpi;	/* Resolution */
  unsigned short *out;
  unsigned char *black,		/* Black bitmap data */
		*cyan,		/* Cyan bitmap data */
		*magenta,	/* Magenta bitmap data */
		*yellow,	/* Yellow bitmap data */
		*lcyan,		/* Light Cyan bitmap data */
		*lmagenta;	/* Light Magenta bitmap data */
  int		page_width,	/* Width of page */
		page_height,	/* Height of page */
		page_left,
		page_top,
		page_right,
		page_bottom,
		out_width,	/* Width of image on page */
		out_height,	/* Height of image on page */
		out_channels,	/* Output bytes per pixel */
		height,		/* Height of raster data */
		errdiv,		/* Error dividend */
		errmod,		/* Error modulus */
		errval,		/* Current error value */
		errline,	/* Current raster line */
		errlast;	/* Last raster line loaded */
  int		zero_mask;
  void		(*writefunc)(const stp_vars_t, unsigned char *, unsigned char *,
		int, int);	/* PCL output function */
  int           image_height,
                image_width,
                image_bpp;
  const pcl_cap_t *caps;		/* Printer capabilities */
  int		do_cret = 0,	/* 300 DPI CRet printing */
  		do_cretb = 0,	/* 600 DPI CRet printing HP 840C*/
		do_6color = 0,	/* CMY + cmK printing */
		planes = 3;	/* # of output planes */
  int		pcl_media_size, /* PCL media size code */
		pcl_media_type, /* PCL media type code */
		pcl_media_source;	/* PCL media source code */
  const double *dot_sizes_use,dot_sizes_cret[]={1.0,1.0,1.0};         /* The dot size used */
  stp_vars_t	nv = stp_allocate_copy(v);
  stp_papersize_t pp;
  int		len_c,		/* Active length of Cyan buffers */
		len_lc,		/* Ditto Light Cyan */
		len_m,		/* Ditto Magenta */
		len_lm,		/* Ditto Light Magenta */
		len_y, 		/* Ditto Cyan */
		len_k;		/* Ditto Black */
  int		blank_lines,	/* Accumulated blank lines */
		is_blank,	/* Current line is blank */
		do_blank;	/* Blank line removal required */
  unsigned char *comp_buf;	/* Scratch buffer for pcl_mode2 */
  int		the_top_margin,	/* Corrected top margin */
		the_left_margin;	/* Corrected left margin */
  stp_curve_t   lum_adjustment;
  stp_curve_t   hue_adjustment;

  if (!stp_verify(nv))
    {
      stp_eprintf(nv, "Print options not verified; cannot print.\n");
      return 0;
    }

  caps = pcl_get_model_capabilities(model);

 /*
  * Setup a read-only pixel region for the entire image...
  */

  stp_image_init(image);
  image_height = stp_image_height(image);
  image_width = stp_image_width(image);
  image_bpp = stp_image_bpp(image);

 /*
  * Figure out the output resolution...
  */

  xdpi = 0;
  ydpi = 0;
  if (resolution)
    {
      for (i = 0; i < NUM_RESOLUTIONS; i++)
	{
	  if (!strcmp(resolution, pcl_resolutions[i].pcl_name))
	    {
	      xdpi = pcl_resolutions[i].p0;
	      ydpi = pcl_resolutions[i].p1;
	      break;
	    }
	}
    }

  stp_deprintf(STP_DBG_PCL,"pcl: resolution=%dx%d\n",xdpi,ydpi);
  if (xdpi == 0 || ydpi == 0)
    return 0;

 /*
  * Choose the correct color conversion function...
  */
  if (((caps->resolutions & PCL_RES_600_600_MONO) == PCL_RES_600_600_MONO) &&
      output_type != OUTPUT_GRAY && xdpi == 600 && ydpi == 600)
    {
      stp_eprintf(v, "600x600 resolution only available in MONO\n");
      output_type = OUTPUT_GRAY;
      stp_set_output_type(nv, OUTPUT_GRAY);
    }

  if (caps->color_type == PCL_COLOR_NONE)
    {
      output_type = OUTPUT_GRAY;
      stp_set_output_type(nv, OUTPUT_GRAY);
    }
  stp_set_output_color_model(nv, COLOR_MODEL_CMY);

  do_cret = (xdpi >= 300 &&
	     ((caps->color_type & PCL_COLOR_CMYK4) == PCL_COLOR_CMYK4));
  do_cretb = (xdpi >= 600 && ydpi >= 600 &&
	      ((caps->color_type & PCL_COLOR_CMYK4b) == PCL_COLOR_CMYK4b) &&
	      output_type != OUTPUT_GRAY);
  if (do_cretb){
    do_cret = 1;
    dot_sizes_use=dot_sizes_cret;
  }else{
    dot_sizes_use=dot_sizes;
  }

  stp_deprintf(STP_DBG_PCL, "do_cret = %d\n", do_cret);
  stp_deprintf(STP_DBG_PCL, "do_cretb = %d\n", do_cretb);

  if (ink_type)
    do_6color = (strcmp(ink_type, "Photo") == 0);
  stp_deprintf(STP_DBG_PCL, "do_6color = %d\n", do_6color);

 /*
  * Compute the output size...
  */

  out_width = stp_get_width(v);
  out_height = stp_get_height(v);

  pcl_imageable_area(nv, &page_left, &page_right, &page_bottom, &page_top);
  left -= page_left;
  top -= page_top;
  page_width = page_right - page_left;
  page_height = page_bottom - page_top;

  image_height = stp_image_height(image);
  image_width = stp_image_width(image);

 /*
  * Let the user know what we're doing...
  */

  stp_image_progress_init(image);

 /*
  * Send PCL initialization commands...
  */

  if (do_cretb)
    {
      stp_puts("\033*rbC", v);	/* End raster graphics */
    }
  stp_puts("\033E", v); 				/* PCL reset */
  if (do_cretb)
    {
      stp_zprintf(v, "\033%%-12345X@PJL ENTER LANGUAGE=PCL3GUI\n");
    }

 /*
  * Set media size
  */

  if (!media_size)
    media_size = "";
  if (strlen(media_size) == 0 &&
      ((pp = stp_get_papersize_by_size(stp_get_page_height(v),
				       stp_get_page_width(v))) != NULL))
    media_size = stp_papersize_get_name(pp);

  pcl_media_size = pcl_convert_media_size(media_size, model);

  stp_deprintf(STP_DBG_PCL,"pcl_media_size = %d, media_size = %s\n", pcl_media_size, media_size);

 /*
  * If the media size requested is unknown, try it as a custom size.
  *
  * Warning: The margins may need to be fixed for this!
  */

  if (pcl_media_size == -1) {
    stp_deprintf(STP_DBG_PCL, "Paper size %s is not directly supported by printer.\n",
      media_size);
    stp_deprintf(STP_DBG_PCL, "Trying as custom pagesize (watch the margins!)\n");
    pcl_media_size = PCL_PAPERSIZE_CUSTOM;			/* Custom */
  }

  stp_zprintf(v, "\033&l%dA", pcl_media_size);

  stp_puts("\033&l0L", v);			/* Turn off perforation skip */
  stp_puts("\033&l0E", v);			/* Reset top margin to 0 */

 /*
  * Convert media source string to the code, if specified.
  */

  if (media_source && strlen(media_source) != 0) {
    pcl_media_source = pcl_string_to_val(media_source, pcl_media_sources,
                         sizeof(pcl_media_sources) / sizeof(pcl_t));

    stp_deprintf(STP_DBG_PCL,"pcl_media_source = %d, media_source = %s\n", pcl_media_source,
           media_source);

    if (pcl_media_source == -1)
      stp_deprintf(STP_DBG_PCL, "Unknown media source %s, ignored.\n", media_source);
    else if (pcl_media_source != PCL_PAPERSOURCE_STANDARD) {

/* Correct the value by taking the modulus */

      pcl_media_source = pcl_media_source % PAPERSOURCE_MOD;
      stp_zprintf(v, "\033&l%dH", pcl_media_source);
    }
  }

 /*
  * Convert media type string to the code, if specified.
  */

  if (media_type && strlen(media_type) != 0) {
    pcl_media_type = pcl_string_to_val(media_type, pcl_media_types,
                       sizeof(pcl_media_types) / sizeof(pcl_t));

    stp_deprintf(STP_DBG_PCL,"pcl_media_type = %d, media_type = %s\n", pcl_media_type,
           media_type);

    if (pcl_media_type == -1) {
      stp_deprintf(STP_DBG_PCL, "Unknown media type %s, set to PLAIN.\n", media_type);
      pcl_media_type = PCL_PAPERTYPE_PLAIN;
    }

/*
 * The HP812C doesn't like glossy paper being selected when using 600x600
 * C-RET (PhotoRET II). So we use Premium paper instead.
 *
 */

    if (do_cretb && pcl_media_type == PCL_PAPERTYPE_GLOSSY) {
      stp_deprintf(STP_DBG_PCL, "Media type GLOSSY, set to PREMIUM for PhotoRET II.\n");
      pcl_media_type = PCL_PAPERTYPE_PREMIUM;
    }
  }
  else
    pcl_media_type = PCL_PAPERTYPE_PLAIN;

 /*
  * Set DJ print quality to "best" if resolution >= 300
  */

  if ((xdpi >= 300) && ((caps->stp_printer_type & PCL_PRINTER_DJ) == PCL_PRINTER_DJ))
  {
    if ((caps->stp_printer_type & PCL_PRINTER_MEDIATYPE) == PCL_PRINTER_MEDIATYPE)
    {
      stp_puts("\033*o1M", v);			/* Quality = presentation */
      stp_zprintf(v, "\033&l%dM", pcl_media_type);
    }
    else
    {
      stp_puts("\033*r2Q", v);			/* Quality (high) */
      stp_puts("\033*o2Q", v);			/* Shingling (4 passes) */

 /* Depletion depends on media type and cart type. */

      if ((pcl_media_type == PCL_PAPERTYPE_PLAIN)
	|| (pcl_media_type == PCL_PAPERTYPE_BOND)) {
      if ((caps->color_type & PCL_COLOR_CMY) == PCL_COLOR_CMY)
          stp_puts("\033*o2D", v);			/* Depletion 25% */
        else
          stp_puts("\033*o5D", v);			/* Depletion 50% with gamma correction */
      }

      else if ((pcl_media_type == PCL_PAPERTYPE_PREMIUM)
             || (pcl_media_type == PCL_PAPERTYPE_GLOSSY)
             || (pcl_media_type == PCL_PAPERTYPE_TRANS))
        stp_puts("\033*o1D", v);			/* Depletion none */
    }
  }

  if ((xdpi != ydpi) || (do_cret) || (do_6color))
						/* Set resolution using CRD */
  {

   /*
    * Send configure image data command with horizontal and
    * vertical resolutions as well as a color count...
    */

    if (output_type != OUTPUT_GRAY)
      if ((caps->color_type & PCL_COLOR_CMY) == PCL_COLOR_CMY)
        planes = 3;
      else
        if (do_6color)
          planes = 6;
        else
          planes = 4;
    else
      planes = 1;

    stp_zprintf(v, "\033*g%dW", 2 + (planes * 6));
    stp_putc(2, v);				/* Format 2 (Complex Direct Planar) */
    stp_putc(planes, v);				/* # output planes */

    if (planes != 3) {
      stp_putc(xdpi >> 8, v);			/* Black resolution */
      stp_putc(xdpi, v);
      stp_putc(ydpi >> 8, v);
      stp_putc(ydpi, v);
      stp_putc(0, v);
      if (do_cretb){
	stp_putc(2, v);
      }else{
	stp_putc(do_cret ? 4 : 2, v);
      }
    }

    if (planes != 1) {
      stp_putc(xdpi >> 8, v);			/* Cyan resolution */
      stp_putc(xdpi, v);
      stp_putc(ydpi >> 8, v);
      stp_putc(ydpi, v);
      stp_putc(0, v);
      stp_putc(do_cret ? 4 : 2, v);

      stp_putc(xdpi >> 8, v);			/* Magenta resolution */
      stp_putc(xdpi, v);
      stp_putc(ydpi >> 8, v);
      stp_putc(ydpi, v);
      stp_putc(0, v);
      stp_putc(do_cret ? 4 : 2, v);

      stp_putc(xdpi >> 8, v);			/* Yellow resolution */
      stp_putc(xdpi, v);
      stp_putc(ydpi >> 8, v);
      stp_putc(ydpi, v);
      stp_putc(0, v);
      stp_putc(do_cret ? 4 : 2, v);
    }
    if (planes == 6)
    {
      stp_putc(xdpi >> 8, v);			/* Light Cyan resolution */
      stp_putc(xdpi, v);
      stp_putc(ydpi >> 8, v);
      stp_putc(ydpi, v);
      stp_putc(0, v);
      stp_putc(do_cret ? 4 : 2, v);

      stp_putc(xdpi >> 8, v);			/* Light Magenta resolution */
      stp_putc(xdpi, v);
      stp_putc(ydpi >> 8, v);
      stp_putc(ydpi, v);
      stp_putc(0, v);
      stp_putc(do_cret ? 4 : 2, v);
    }
  }
  else
  {
    stp_zprintf(v, "\033*t%dR", xdpi);		/* Simple resolution */
    if (output_type != OUTPUT_GRAY)
    {
      if ((caps->color_type & PCL_COLOR_CMY) == PCL_COLOR_CMY)
        stp_puts("\033*r-3U", v);		/* Simple CMY color */
      else
        stp_puts("\033*r-4U", v);		/* Simple KCMY color */
    }
  }

#ifndef PCL_DEBUG_DISABLE_COMPRESSION
  if ((caps->stp_printer_type & PCL_PRINTER_TIFF) == PCL_PRINTER_TIFF)
  {
    stp_puts("\033*b2M", v);			/* Mode 2 (TIFF) */
  }
  else
#endif
  {
    stp_puts("\033*b0M", v);			/* Mode 0 (no compression) */
  }

 /*
  * Convert image size to printer resolution and setup the page for printing...
  */

  out_width  = xdpi * out_width / 72;
  out_height = ydpi * out_height / 72;

  if (pcl_media_size == PCL_PAPERSIZE_A4)
  {
    the_left_margin = caps->a4_margins.left_margin;
    the_top_margin = caps->a4_margins.top_margin;
  }
  else
  {
    the_left_margin = caps->normal_margins.left_margin;
    the_top_margin = caps->normal_margins.top_margin;
  }

  stp_deprintf(STP_DBG_PCL, "left %d margin %d top %d margin %d width %d height %d\n",
	  left, the_left_margin, top, the_top_margin, out_width, out_height);

  if (!do_cretb) {
    stp_zprintf(v, "\033&a%dH", 10 * left);		/* Set left raster position */
    stp_zprintf(v, "\033&a%dV", 10 * (top + the_top_margin));
				/* Set top raster position */
  }
  stp_zprintf(v, "\033*r%dS", out_width);		/* Set raster width */
  stp_zprintf(v, "\033*r%dT", out_height);	/* Set raster height */

  if (do_cretb)
    {
      /* Move to top left of printed area */
      stp_zprintf(v, "\033*p%dY", (top + the_top_margin)*4); /* Measured in dots. */
      stp_zprintf(v, "\033*p%dX", left*4);
    }
  stp_puts("\033*r1A", v); 			/* Start GFX */

 /*
  * Allocate memory for the raster data...
  */

  height = (out_width + 7) / 8;
  if (do_cret)
    height *= 2;

  if (output_type == OUTPUT_GRAY)
  {
    black   = stp_malloc(height);
    cyan    = NULL;
    magenta = NULL;
    yellow  = NULL;
    lcyan    = NULL;
    lmagenta = NULL;
  }
  else
  {
    cyan    = stp_malloc(height);
    magenta = stp_malloc(height);
    yellow  = stp_malloc(height);

    if ((caps->color_type & PCL_COLOR_CMY) == PCL_COLOR_CMY)
      black = NULL;
    else
      black = stp_malloc(height);
    if (do_6color)
    {
      lcyan    = stp_malloc(height);
      lmagenta = stp_malloc(height);
    }
    else
    {
      lcyan    = NULL;
      lmagenta = NULL;
    }
  }

/* Allocate buffer for pcl_mode2 tiff compression */

#ifndef PCL_DEBUG_DISABLE_COMPRESSION
  if ((caps->stp_printer_type & PCL_PRINTER_TIFF) == PCL_PRINTER_TIFF)
  {
    comp_buf = stp_malloc((height + 128 + 7) * 129 / 128);
    writefunc = pcl_mode2;
  }
  else
#endif
  {
    comp_buf = NULL;
    writefunc = pcl_mode0;
  }

/* Set up dithering for special printers. */

#if 1		/* Leave alone for now */
  stp_set_default_float_parameter(nv, "GCRLower", .3);
  stp_set_default_float_parameter(nv, "GCRUpper", .999);
#endif
  stp_dither_init(nv, image, out_width, xdpi, ydpi);

/* Ensure that density does not exceed 1.0 */

  stp_deprintf(STP_DBG_PCL, "Density: %f\n", stp_get_float_parameter(nv, "Density"));
  if (stp_get_float_parameter(nv, "Density") > 1.0)
    stp_set_float_parameter(nv, "Density", 1.0);

  if (do_cret)				/* 4-level printing for 800/1120 */
    {
      stp_dither_set_ranges_simple(nv, ECOLOR_Y, 3, dot_sizes_use, stp_get_float_parameter(nv, "Density"));
      if (!do_cretb)
        stp_dither_set_ranges_simple(nv, ECOLOR_K, 3, dot_sizes_use, stp_get_float_parameter(nv, "Density"));

/* Note: no printer I know of does both CRet (4-level) and 6 colour, but
   what the heck. variable_dither_ranges copied from print-escp2.c */

      if (do_6color)			/* Photo for 69x */
	{
	  stp_dither_set_ranges(nv, ECOLOR_C, 6, variable_dither_ranges,
			    stp_get_float_parameter(nv, "Density"));
	  stp_dither_set_ranges(nv, ECOLOR_M, 6, variable_dither_ranges,
			    stp_get_float_parameter(nv, "Density"));
	}
      else
	{
	  stp_dither_set_ranges_simple(nv, ECOLOR_C, 3, dot_sizes_use, stp_get_float_parameter(nv, "Density"));
	  stp_dither_set_ranges_simple(nv, ECOLOR_M, 3, dot_sizes_use, stp_get_float_parameter(nv, "Density"));
	}
    }
  else if (do_6color)
    {
/* Set light inks for 6 colour printers. Numbers copied from print-escp2.c */
      stp_dither_set_light_ink(nv, ECOLOR_C, .25, stp_get_float_parameter(nv, "Density"));
      stp_dither_set_light_ink(nv, ECOLOR_M, .25, stp_get_float_parameter(nv, "Density"));
    }

  if (!stp_check_curve_parameter(nv, "HueMap"))
    {
      hue_adjustment = stp_curve_allocate_read_string(standard_hue_adjustment);
      stp_set_curve_parameter(nv, "HueMap", hue_adjustment);
      stp_curve_destroy(hue_adjustment);
    }
  if (!stp_check_curve_parameter(nv, "LumMap"))
    {
      lum_adjustment = stp_curve_allocate_read_string(standard_lum_adjustment);
      stp_curve_destroy(lum_adjustment);
    }
  if (output_type == OUTPUT_COLOR && black)
    {
      output_type = OUTPUT_RAW_CMYK;
      stp_set_output_type(nv, OUTPUT_RAW_CMYK);
    }

  out_channels = stp_color_init(nv, image, 65536);

  out = stp_malloc(image_width * out_channels * 2);

  errdiv  = image_height / out_height;
  errmod  = image_height % out_height;
  errval  = 0;
  errlast = -1;
  errline  = 0;
  blank_lines = 0;
  is_blank = 0;
#ifndef PCL_DEBUG_DISABLE_BLANKLINE_REMOVAL
  do_blank = ((caps->stp_printer_type & PCL_PRINTER_BLANKLINE) ==
		PCL_PRINTER_BLANKLINE);
#else
  do_blank = 0;
#endif

  stp_dither_add_channel(nv, black, ECOLOR_K, 0);
  stp_dither_add_channel(nv, cyan, ECOLOR_C, 0);
  stp_dither_add_channel(nv, lcyan, ECOLOR_C, 1);
  stp_dither_add_channel(nv, magenta, ECOLOR_M, 0);
  stp_dither_add_channel(nv, lmagenta, ECOLOR_M, 1);
  stp_dither_add_channel(nv, yellow, ECOLOR_Y, 0);

  for (y = 0; y < out_height; y ++)
  {
    int duplicate_line = 1;
    if ((y & 63) == 0)
      stp_image_note_progress(image, y, out_height);
    if (errline != errlast)
    {
      errlast = errline;
      duplicate_line = 0;
      if (stp_color_get_row(nv, image, errline, out, &zero_mask))
	{
	  status = 2;
	  break;
	}
    }
    stp_dither(nv, y, out, duplicate_line, zero_mask);
    len_c = stp_dither_get_last_position(nv, ECOLOR_C, 0);
    len_lc = stp_dither_get_last_position(nv, ECOLOR_C, 1);
    len_m = stp_dither_get_last_position(nv, ECOLOR_M, 0);
    len_lm = stp_dither_get_last_position(nv, ECOLOR_M, 1);
    len_y = stp_dither_get_last_position(nv, ECOLOR_Y, 0);
    len_k = stp_dither_get_last_position(nv, ECOLOR_K, 0);

/*
 * Blank line removal. If multiple lines are blank then they can be replaced
 * by "Relative Vertical Pixel Movement" command. However, since there are
 * apparently some faulty implementations, we always output the first line,
 * then suppress the rest. This ensures that the printers's buffers are really
 * empty. As suggested by Mike Sweet.
 */

    is_blank = (do_blank && (len_c == -1) && (len_lc == -1) && (len_m == -1)
	&& (len_lm == -1) && (len_y == -1) && (len_k == -1));
    stp_deprintf(STP_DBG_PCL,"pcl_print: y = %d, line = %d, val = %d, mod = %d, height = %d\n",
           y, errline, errval, errmod, out_height);

    if (is_blank && (blank_lines != 0))	/* repeated blank line */
    {
      blank_lines++;
    }
    else				/* Not blank, or is first one */
    {
      if (! is_blank)
      {
        if (blank_lines > 1)		/* Output accumulated lines */
        {
	  blank_lines--;		/* correct for one already output */
	  stp_deprintf(STP_DBG_PCL, "Blank Lines = %d\n", blank_lines);
	  stp_zprintf(v, "\033*b%dY", blank_lines);
	  blank_lines=0;
        }
	else;
      }
      else
      {
	blank_lines++;
      }

      if (do_cret)
      {
       /*
        * 4-level (CRet) dithers...
        */
        if (output_type == OUTPUT_GRAY)
        {
          (*writefunc)(v, comp_buf, black + height / 2, height / 2, 0);
          (*writefunc)(v, comp_buf, black, height / 2, 1);
        }
        else
        {
	  if(do_cretb)
	  {
/*	    (*writefunc)(v, comp_buf, black + height / 2, 0, 0); */
	    (*writefunc)(v, comp_buf, black, height/2, 0);
	  }
	  else
	  {
	    (*writefunc)(v, comp_buf, black + height / 2, height / 2, 0);
	    (*writefunc)(v, comp_buf, black, height / 2, 0);
	  }
          (*writefunc)(v, comp_buf, cyan + height / 2, height / 2, 0);
          (*writefunc)(v, comp_buf, cyan, height / 2, 0);
          (*writefunc)(v, comp_buf, magenta + height / 2, height / 2, 0);
          (*writefunc)(v, comp_buf, magenta, height / 2, 0);
          (*writefunc)(v, comp_buf, yellow + height / 2, height / 2, 0);
          if (do_6color)
          {
            (*writefunc)(v, comp_buf, yellow, height / 2, 0);
            (*writefunc)(v, comp_buf, lcyan + height / 2, height / 2, 0);
            (*writefunc)(v, comp_buf, lcyan, height / 2, 0);
            (*writefunc)(v, comp_buf, lmagenta + height / 2, height / 2, 0);
            (*writefunc)(v, comp_buf, lmagenta, height / 2, 1);		/* Last plane set on light magenta */
          }
          else
            (*writefunc)(v, comp_buf, yellow, height / 2, 1);		/* Last plane set on yellow */
        }
      }
      else
      {
       /*
        * Standard 2-level dithers...
        */

        if (output_type == OUTPUT_GRAY)
        {
          (*writefunc)(v, comp_buf, black, height, 1);
        }
        else
        {
          if (black != NULL)
            (*writefunc)(v, comp_buf, black, height, 0);
          (*writefunc)(v, comp_buf, cyan, height, 0);
          (*writefunc)(v, comp_buf, magenta, height, 0);
          if (do_6color)
          {
            (*writefunc)(v, comp_buf, yellow, height, 0);
            (*writefunc)(v, comp_buf, lcyan, height, 0);
            (*writefunc)(v, comp_buf, lmagenta, height, 1);		/* Last plane set on light magenta */
          }
          else
            (*writefunc)(v, comp_buf, yellow, height, 1);		/* Last plane set on yellow */
        }
      }
    }
    stp_deprintf(STP_DBG_PCL,"pcl_print: y = %d, line = %d, val = %d, mod = %d, height = %d\n",
           y, errline, errval, errmod, out_height);

    errval += errmod;
    errline += errdiv;
    if (errval >= out_height)
    {
      errval -= out_height;
      errline ++;
    }
  }

/* Output trailing blank lines (may not be required?) */

  if (blank_lines > 1)
  {
    blank_lines--;		/* correct for one already output */
    stp_deprintf(STP_DBG_PCL, "Blank Lines = %d\n", blank_lines);
    stp_zprintf(v, "\033*b%dY", blank_lines);
    blank_lines=0;
  }

  stp_image_progress_conclude(image);

  stp_dither_free(nv);


 /*
  * Cleanup...
  */

  stp_free(out);

  if (black != NULL)
    stp_free(black);
  if (cyan != NULL)
  {
    stp_free(cyan);
    stp_free(magenta);
    stp_free(yellow);
  }
  if (lcyan != NULL)
  {
    stp_free(lcyan);
    stp_free(lmagenta);
  }

  if (comp_buf != NULL)
    stp_free(comp_buf);

  if ((caps->stp_printer_type & PCL_PRINTER_NEW_ERG) == PCL_PRINTER_NEW_ERG)
    stp_puts("\033*rC", v);
  else
    stp_puts("\033*rB", v);

  stp_puts("\033&l0H", v);		/* Eject page */
  if (do_cretb)
    {
      stp_zprintf(v, "\033%%-12345X\n");
    }
  stp_puts("\033E", v); 				/* PCL reset */
  stp_vars_free(nv);
  return status;
}

static const stp_printfuncs_t stp_pcl_printfuncs =
{
  pcl_list_parameters,
  pcl_parameters,
  stp_default_media_size,
  pcl_imageable_area,
  pcl_limit,
  pcl_print,
  pcl_describe_resolution,
  stp_verify_printer_params,
  NULL,
  NULL
};


/*
 * 'pcl_mode0()' - Send PCL graphics using mode 0 (no) compression.
 */

static void
pcl_mode0(const stp_vars_t v,		/* I - Print file or command */
          unsigned char *comp_buf,	/* I - scratch buffer (not used) */
          unsigned char *line,		/* I - Output bitmap data */
          int           height,		/* I - Height of bitmap data */
          int           last_plane)	/* I - True if this is the last plane */
{
  stp_zprintf(v, "\033*b%d%c", height, last_plane ? 'W' : 'V');
  stp_zfwrite((const char *) line, height, 1, v);
}


/*
 * 'pcl_mode2()' - Send PCL graphics using mode 2 (TIFF) compression.
 */

static void
pcl_mode2(const stp_vars_t v,		/* I - Print file or command */
          unsigned char *comp_buf,	/* I - Scratch Buffer */
          unsigned char *line,		/* I - Output bitmap data */
          int           height,		/* I - Height of bitmap data */
          int           last_plane)	/* I - True if this is the last plane */
{
  unsigned char	*comp_ptr;		/* Current slot in buffer */

  stp_pack_tiff(line, height, comp_buf, &comp_ptr, NULL, NULL);

 /*
  * Send a line of raster graphics...
  */

  stp_zprintf(v, "\033*b%d%c", (int)(comp_ptr - comp_buf), last_plane ? 'W' : 'V');
  stp_zfwrite((const char *)comp_buf, comp_ptr - comp_buf, 1, v);
}


static stp_internal_family_t stp_pcl_module_data =
  {
    &stp_pcl_printfuncs,
    NULL
  };


static int
pcl_module_init(void)
{
  return stp_family_register(stp_pcl_module_data.printer_list);
}


static int
pcl_module_exit(void)
{
  return stp_family_unregister(stp_pcl_module_data.printer_list);
}


/* Module header */
#define stp_module_version pcl_LTX_stp_module_version
#define stp_module_data pcl_LTX_stp_module_data

stp_module_version_t stp_module_version = {0, 0};

stp_module_t stp_module_data =
  {
    "pcl",
    VERSION,
    "PCL family driver",
    STP_MODULE_CLASS_FAMILY,
    NULL,
    pcl_module_init,
    pcl_module_exit,
    (void *) &stp_pcl_module_data
  };

