/*
 * "$Id: mxml.h,v 1.1 2004/04/25 12:17:49 rleigh Exp $"
 *
 * Header file for mini-XML, a small XML-like file parsing library.
 *
 * Copyright 2003 by Michael Sweet.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * Prevent multiple inclusion...
 */

#ifndef GIMP_PRINT_MXML_H
#  define GIMP_PRINT_MXML_H

/*
 * Include necessary headers...
 */

#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#  include <ctype.h>
#  include <errno.h>


/*
 * Constants...
 */

#  define MXML_WRAP		70	/* Wrap XML output at this column position */
#  define MXML_TAB		8	/* Tabs every N columns */

#  define MXML_NO_CALLBACK	0	/* Don't use a type callback */
#  define MXML_NO_PARENT	0	/* No parent for the node */

#  define MXML_DESCEND		1	/* Descend when finding/walking */
#  define MXML_NO_DESCEND	0	/* Don't descend when finding/walking */
#  define MXML_DESCEND_FIRST	-1	/* Descend for first find */

#  define MXML_WS_BEFORE_OPEN	0	/* Callback for before open tag */
#  define MXML_WS_AFTER_OPEN	1	/* Callback for after open tag */
#  define MXML_WS_BEFORE_CLOSE	2	/* Callback for before close tag */
#  define MXML_WS_AFTER_CLOSE	3	/* Callback for after close tag */

#  define MXML_ADD_BEFORE	0	/* Add node before specified node */
#  define MXML_ADD_AFTER	1	/* Add node after specified node */
#  define MXML_ADD_TO_PARENT	NULL	/* Add node relative to parent */


/*
 * Data types...
 */

typedef enum mxml_type_e		/**** The XML node type. ****/
{
  MXML_ELEMENT,				/* XML element with attributes */
  MXML_INTEGER,				/* Integer value */
  MXML_OPAQUE,				/* Opaque string */
  MXML_REAL,				/* Real value */
  MXML_TEXT				/* Text fragment */
} mxml_type_t;

typedef struct mxml_attr_s		/**** An XML element attribute value. ****/
{
  char	*name;				/* Attribute name */
  char	*value;				/* Attribute value */
} mxml_attr_t;

typedef struct mxml_value_s		/**** An XML element value. ****/
{
  char		*name;			/* Name of element */
  int		num_attrs;		/* Number of attributes */
  mxml_attr_t	*attrs;			/* Attributes */
} mxml_element_t;

typedef struct mxml_text_s		/**** An XML text value. ****/
{
  int		whitespace;		/* Leading whitespace? */
  char		*string;		/* Fragment string */
} mxml_text_t;

typedef union mxml_value_u		/**** An XML node value. ****/
{
  mxml_element_t	element;	/* Element */
  int			integer;	/* Integer number */
  char			*opaque;	/* Opaque string */
  double		real;		/* Real number */
  mxml_text_t		text;		/* Text fragment */
} mxml_value_t;

typedef struct mxml_node_s mxml_node_t;	/**** An XML node. ****/

struct mxml_node_s			/**** An XML node. ****/
{
  mxml_type_t	type;			/* Node type */
  mxml_node_t	*next;			/* Next node under same parent */
  mxml_node_t	*prev;			/* Previous node under same parent */
  mxml_node_t	*parent;		/* Parent node */
  mxml_node_t	*child;			/* First child node */
  mxml_node_t	*last_child;		/* Last child node */
  mxml_value_t	value;			/* Node value */
};


/*
 * C++ support...
 */

#  ifdef __cplusplus
extern "C" {
#  endif /* __cplusplus */

/*
 * Prototypes...
 */

extern void		stpi_mxmlAdd(mxml_node_t *parent, int where,
			        mxml_node_t *child, mxml_node_t *node);
extern void		stpi_mxmlDelete(mxml_node_t *node);
extern const char	*stpi_mxmlElementGetAttr(mxml_node_t *node, const char *name);
extern void		stpi_mxmlElementSetAttr(mxml_node_t *node, const char *name,
			                   const char *value);
extern mxml_node_t	*stpi_mxmlFindElement(mxml_node_t *node, mxml_node_t *top,
			                 const char *name, const char *attr,
					 const char *value, int descend);
extern mxml_node_t	*stpi_mxmlLoadFile(mxml_node_t *top, FILE *fp,
			              mxml_type_t (*cb)(mxml_node_t *));
extern mxml_node_t	*stpi_mxmlLoadString(mxml_node_t *top, const char *s,
			                mxml_type_t (*cb)(mxml_node_t *));
extern mxml_node_t	*stpi_mxmlNewElement(mxml_node_t *parent, const char *name);
extern mxml_node_t	*stpi_mxmlNewInteger(mxml_node_t *parent, int integer);
extern mxml_node_t	*stpi_mxmlNewOpaque(mxml_node_t *parent, const char *opaque);
extern mxml_node_t	*stpi_mxmlNewReal(mxml_node_t *parent, double real);
extern mxml_node_t	*stpi_mxmlNewText(mxml_node_t *parent, int whitespace,
			             const char *string);
extern void		stpi_mxmlRemove(mxml_node_t *node);
extern char		*stpi_mxmlSaveAllocString(mxml_node_t *node,
			        	     int (*cb)(mxml_node_t *, int));
extern int		stpi_mxmlSaveFile(mxml_node_t *node, FILE *fp,
			             int (*cb)(mxml_node_t *, int));
extern int		stpi_mxmlSaveString(mxml_node_t *node, char *buffer,
			               int bufsize,
			               int (*cb)(mxml_node_t *, int));
extern mxml_node_t	*stpi_mxmlWalkNext(mxml_node_t *node, mxml_node_t *top,
			              int descend);
extern mxml_node_t	*stpi_mxmlWalkPrev(mxml_node_t *node, mxml_node_t *top,
			              int descend);


/*
 * C++ support...
 */

#  ifdef __cplusplus
}
#  endif /* __cplusplus */
#endif /* !GIMP_PRINT_MXML_H */


/*
 * End of "$Id: mxml.h,v 1.1 2004/04/25 12:17:49 rleigh Exp $".
 */
