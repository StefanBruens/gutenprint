/*
 * "$Id: xml.h,v 1.8 2003/06/19 23:09:22 rlk Exp $"
 *
 *   libgimpprint module loader header
 *
 *   Copyright 1997-2002 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and Michael Natterer (mitch@gimp.org)
 *   Copyright 2002-2003 Roger Leigh (roger@whinlatter.uklinux.net)
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


#ifndef GIMP_PRINT_INTERNAL_XML_H
#define GIMP_PRINT_INTERNAL_XML_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mxml.h"

typedef int (*stpi_xml_parse_func)(mxml_node_t *node, const char *file);

extern void
stpi_register_xml_parser(const char *name, stpi_xml_parse_func parse_func);

extern void
stpi_unregister_xml_parser(const char *name);

extern int stpi_xml_init_defaults(void);
extern int stpi_xml_parse_file(const char *file);

extern long stpi_xmlstrtol(const char *value);
extern unsigned long stpi_xmlstrtoul(const char *value);
extern double stpi_xmlstrtod(const char *textval);

extern void stpi_xml_init(void);
extern void stpi_xml_exit(void);
extern mxml_node_t *stpi_xml_get_node(mxml_node_t *xmlroot, ...);
extern mxml_node_t *stpi_xmldoc_create_generic(void);
extern void stpi_xml_preinit(void);

extern stp_sequence_t stpi_sequence_create_from_xmltree(mxml_node_t *da);
extern mxml_node_t *stpi_xmltree_create_from_sequence(stp_sequence_t seq);

extern stp_array_t stpi_array_create_from_xmltree(mxml_node_t *array);

#endif /* GIMP_PRINT_INTERNAL_XML_H */
/*
 * End of "$Id: xml.h,v 1.8 2003/06/19 23:09:22 rlk Exp $".
 */
