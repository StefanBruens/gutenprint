/*
 * "$Id: intl.c,v 1.11 2008/07/24 23:50:54 rlk Exp $"
 *
 *   Additional Gutenprint localization code.
 *
 *   Copyright 2008 Michael R Sweet
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
#include <string.h>
#include <gutenprint/gutenprint-intl-internal.h>

#ifdef ENABLE_NLS
#  ifdef __APPLE__
/*
 * Mac OS X uses the CoreFoundation framework to support localization via
 * language resource directories and files that are typically installed in
 * /Library/Printers/Gutenprint/Contents/Resources.
 *
 * We use a single function to simulate the gettext() and other functions
 * provided in libc/libintl on GNU/Linux.
 */

const char *				/* O - Localized version of string */
stp_cfgettext(const char *string)	/* I - Original English string */
{
  CFStringRef	cfstring;		/* Copy of English string */
  CFStringRef	cflocstring;		/* Copy of localized string */
  const char	*locstring;		/* Localized UTF-8 string */
  static int	bundled = -1;		/* Is this app bundled? */


  if (bundled < 0)
    bundled = getenv("CFProcessPath") != NULL;

  if (!string || !bundled)
    return (string);

  if ((cfstring = CFStringCreateWithCString(NULL, string, kCFStringEncodingUTF8)) == NULL)
    return (string);

  if ((cflocstring = CFCopyLocalizedString(cfstring, NULL)) == NULL)
    return (string);

  locstring = CFStringGetCStringPtr(cflocstring, kCFStringEncodingUTF8);
  CFRelease(cfstring);

  return (locstring ? locstring : string);
}


/*
 * This function sets the POSIX locale and selects the proper CF locale.
 */

const char *				/* O - New locale */
stp_setlocale(const char *lang)		/* I - Locale name or "" for default */
{
  const char	*apple_language;	/* APPLE_LANGUAGE environment variable */
  CFStringRef	language;		/* Language string */
  CFArrayRef	languageArray;		/* Language array */


 /*
  * Setup the Core Foundation language environment for localized messages.
  */

  if (*lang)
    apple_language = lang;
  else
    apple_language = getenv("APPLE_LANGUAGE");

  lang = setlocale(LC_MESSAGES, lang);

  if (!apple_language)
    apple_language = lang;

  if (apple_language)
  {
    language      = CFStringCreateWithCString(kCFAllocatorDefault, apple_language, kCFStringEncodingUTF8);
    languageArray = CFArrayCreate(kCFAllocatorDefault, (const void **)&language, 1, &kCFTypeArrayCallBacks);

    CFPreferencesSetAppValue(CFSTR("AppleLanguages"), languageArray, kCFPreferencesCurrentApplication);
  }

  return (lang);
}

#  else
typedef struct {
  const char *lang;
  const char *mapping;
} locale_map;

static const locale_map lang_mappings[] =
  {
    { "cs", "cs_CZ" },
    { "da", "da_DK" },
    { "de", "de_DE" },
    { "el", "el_GR" },
    { "es", "es_ES" },
    { "fr", "fr_FR" },
    { "hu", "hu_HU" },
    { "ja", "ja_JP" },
    { "nb", "nb_NO" },
    { "nl", "nl_NL" },
    { "pl", "pl_PL" },
    { "pt", "pt_PT" },
    { "sk", "sk_SK" },
    { "sv", "sv_SE" },
    { "cs.UTF8", "cs_CZ.UTF8" },
    { "da.UTF8", "da_DK.UTF8" },
    { "de.UTF8", "de_DE.UTF8" },
    { "el.UTF8", "el_GR.UTF8" },
    { "es.UTF8", "es_ES.UTF8" },
    { "fr.UTF8", "fr_FR.UTF8" },
    { "hu.UTF8", "hu_HU.UTF8" },
    { "ja.UTF8", "ja_JP.UTF8" },
    { "nb.UTF8", "nb_NO.UTF8" },
    { "nl.UTF8", "nl_NL.UTF8" },
    { "pl.UTF8", "pl_PL.UTF8" },
    { "pt.UTF8", "pt_PT.UTF8" },
    { "sk.UTF8", "sk_SK.UTF8" },
    { "sv.UTF8", "sv_SE.UTF8" },
  };
static int lang_map_count = sizeof(lang_mappings) / sizeof(locale_map);


/*
 * This function sets the POSIX locale.
 */

const char *				/* O - New locale */
stp_setlocale(const char *lang)		/* I - Locale name or "" for default */
{
  char *l = setlocale(LC_ALL, lang ? lang : "");

  /* Make sure the locale we tried to set was accepted! */
  if (lang && !l)
    {
      int i;
      for (i = 0; i < lang_map_count; i++)
	{
	  const locale_map *lm = &(lang_mappings[i]);
	  if (!strcmp(lang, lm->lang))
	    {
	      l = setlocale(LC_ALL, lm->mapping);
	      if (l)
		break;
	    }
	}
    }

#    ifdef LC_CTYPE
  setlocale(LC_CTYPE, l ? l : "");
#    endif /* LC_CTYPE */
#    ifdef LC_NUMERIC
  setlocale(LC_NUMERIC, "C");
#    endif /* LC_NUMERIC */

  return (l);
}
#  endif /* __APPLE__ */
#endif /* ENABLE_NLS */
