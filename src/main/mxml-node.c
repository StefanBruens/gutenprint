/*
 * "$Id: mxml-node.c,v 1.5 2004/04/25 12:17:51 rleigh Exp $"
 *
 * Node support code for mini-XML, a small XML-like file parsing library.
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
 *
 * Contents:
 *
 *   stpi_mxmlAdd()        - Add a node to a tree.
 *   stpi_mxmlDelete()     - Delete a node and all of its children.
 *   stpi_mxmlNewElement() - Create a new element node.
 *   stpi_mxmlNewInteger() - Create a new integer node.
 *   stpi_mxmlNewOpaque()  - Create a new opaque string.
 *   stpi_mxmlNewReal()    - Create a new real number node.
 *   stpi_mxmlNewText()    - Create a new text fragment node.
 *   stpi_mxmlRemove()     - Remove a node from its parent.
 *   mxml_new()       - Create a new node.
 */

/*
 * Include necessary headers...
 */

#include <gimp-print/mxml.h>
#include "config.h"


/*
 * Local functions...
 */

static mxml_node_t	*mxml_new(mxml_node_t *parent, mxml_type_t type);


/*
 * 'stpi_mxmlAdd()' - Add a node to a tree.
 *
 * Adds the specified node to the parent. If the child argument is not
 * NULL, puts the new node before or after the specified child depending
 * on the value of the where argument. If the child argument is NULL,
 * puts the new node at the beginning of the child list (MXML_ADD_BEFORE)
 * or at the end of the child list (MXML_ADD_AFTER). The constant
 * MXML_ADD_TO_PARENT can be used to specify a NULL child pointer.
 */

void
stpi_mxmlAdd(mxml_node_t *parent,		/* I - Parent node */
        int         where,		/* I - Where to add, MXML_ADD_BEFORE or MXML_ADD_AFTER */
        mxml_node_t *child,		/* I - Child node for where or MXML_ADD_TO_PARENT */
	mxml_node_t *node)		/* I - Node to add */
{
/*  fprintf(stderr, "stpi_mxmlAdd(parent=%p, where=%d, child=%p, node=%p)\n", parent,
         where, child, node);*/

 /*
  * Range check input...
  */

  if (!parent || !node)
    return;

 /*
  * Remove the node from any existing parent...
  */

  if (node->parent)
    stpi_mxmlRemove(node);

 /*
  * Reset pointers...
  */

  node->parent = parent;

  switch (where)
  {
    case MXML_ADD_BEFORE :
        if (!child || child == parent->child || child->parent != parent)
	{
	 /*
	  * Insert as first node under parent...
	  */

	  node->next = parent->child;

	  if (parent->child)
	    parent->child->prev = node;
	  else
	    parent->last_child = node;

	  parent->child = node;
	}
	else
	{
	 /*
	  * Insert node before this child...
	  */

	  node->next = child;
	  node->prev = child->prev;

	  if (child->prev)
	    child->prev->next = node;
	  else
	    parent->child = node;

	  child->prev = node;
	}
        break;

    case MXML_ADD_AFTER :
        if (!child || child == parent->last_child || child->parent != parent)
	{
	 /*
	  * Insert as last node under parent...
	  */

	  node->parent = parent;
	  node->prev   = parent->last_child;

	  if (parent->last_child)
	    parent->last_child->next = node;
	  else
	    parent->child = node;

	  parent->last_child = node;
        }
	else
	{
	 /*
	  * Insert node after this child...
	  */

	  node->prev = child;
	  node->next = child->next;

	  if (child->next)
	    child->next->prev = node;
	  else
	    parent->last_child = node;

	  child->next = node;
	}
        break;
  }
}


/*
 * 'stpi_mxmlDelete()' - Delete a node and all of its children.
 *
 * If the specified node has a parent, this function first removes the
 * node from its parent using the stpi_mxmlRemove() function.
 */

void
stpi_mxmlDelete(mxml_node_t *node)		/* I - Node to delete */
{
  int	i;				/* Looping var */


/*  fprintf(stderr, "stpi_mxmlDelete(node=%p)\n", node);*/

 /*
  * Range check input...
  */

  if (!node)
    return;

 /*
  * Remove the node from its parent, if any...
  */

  stpi_mxmlRemove(node);

 /*
  * Delete children...
  */

  while (node->child)
    stpi_mxmlDelete(node->child);

 /*
  * Now delete any node data...
  */

  switch (node->type)
  {
    case MXML_ELEMENT :
        if (node->value.element.name)
	  free(node->value.element.name);

	if (node->value.element.num_attrs)
	{
	  for (i = 0; i < node->value.element.num_attrs; i ++)
	  {
	    if (node->value.element.attrs[i].name)
	      free(node->value.element.attrs[i].name);
	    if (node->value.element.attrs[i].value)
	      free(node->value.element.attrs[i].value);
	  }

          free(node->value.element.attrs);
	}
        break;
    case MXML_INTEGER :
       /* Nothing to do */
        break;
    case MXML_OPAQUE :
        if (node->value.opaque)
	  free(node->value.opaque);
        break;
    case MXML_REAL :
       /* Nothing to do */
        break;
    case MXML_TEXT :
        if (node->value.text.string)
	  free(node->value.text.string);
        break;
  }

 /*
  * Free this node...
  */

  free(node);
}


/*
 * 'stpi_mxmlNewElement()' - Create a new element node.
 *
 * The new element node is added to the end of the specified parent's child
 * list. The constant MXML_NO_PARENT can be used to specify that the new
 * element node has no parent.
 */

mxml_node_t *				/* O - New node */
stpi_mxmlNewElement(mxml_node_t *parent,	/* I - Parent node or MXML_NO_PARENT */
               const char  *name)	/* I - Name of element */
{
  mxml_node_t	*node;			/* New node */


 /*
  * Range check input...
  */

  if (!name)
    return (NULL);

 /*
  * Create the node and set the element name...
  */

  if ((node = mxml_new(parent, MXML_ELEMENT)) != NULL)
    node->value.element.name = strdup(name);

  return (node);
}


/*
 * 'stpi_mxmlNewInteger()' - Create a new integer node.
 *
 * The new integer node is added to the end of the specified parent's child
 * list. The constant MXML_NO_PARENT can be used to specify that the new
 * integer node has no parent.
 */

mxml_node_t *				/* O - New node */
stpi_mxmlNewInteger(mxml_node_t *parent,	/* I - Parent node or MXML_NO_PARENT */
               int         integer)	/* I - Integer value */
{
  mxml_node_t	*node;			/* New node */


 /*
  * Range check input...
  */

  if (!parent)
    return (NULL);

 /*
  * Create the node and set the element name...
  */

  if ((node = mxml_new(parent, MXML_INTEGER)) != NULL)
    node->value.integer = integer;

  return (node);
}


/*
 * 'stpi_mxmlNewOpaque()' - Create a new opaque string.
 *
 * The new opaque node is added to the end of the specified parent's child
 * list. The constant MXML_NO_PARENT can be used to specify that the new
 * opaque node has no parent. The opaque string must be nul-terminated and
 * is copied into the new node.
 */

mxml_node_t *				/* O - New node */
stpi_mxmlNewOpaque(mxml_node_t *parent,	/* I - Parent node or MXML_NO_PARENT */
              const char  *opaque)	/* I - Opaque string */
{
  mxml_node_t	*node;			/* New node */


 /*
  * Range check input...
  */

  if (!parent || !opaque)
    return (NULL);

 /*
  * Create the node and set the element name...
  */

  if ((node = mxml_new(parent, MXML_OPAQUE)) != NULL)
    node->value.opaque = strdup(opaque);

  return (node);
}


/*
 * 'stpi_mxmlNewReal()' - Create a new real number node.
 *
 * The new real number node is added to the end of the specified parent's
 * child list. The constant MXML_NO_PARENT can be used to specify that
 * the new real number node has no parent.
 */

mxml_node_t *				/* O - New node */
stpi_mxmlNewReal(mxml_node_t *parent,	/* I - Parent node or MXML_NO_PARENT */
            double      real)		/* I - Real number value */
{
  mxml_node_t	*node;			/* New node */


 /*
  * Range check input...
  */

  if (!parent)
    return (NULL);

 /*
  * Create the node and set the element name...
  */

  if ((node = mxml_new(parent, MXML_REAL)) != NULL)
    node->value.real = real;

  return (node);
}


/*
 * 'stpi_mxmlNewText()' - Create a new text fragment node.
 *
 * The new text node is added to the end of the specified parent's child
 * list. The constant MXML_NO_PARENT can be used to specify that the new
 * text node has no parent. The whitespace parameter is used to specify
 * whether leading whitespace is present before the node. The text
 * string must be nul-terminated and is copied into the new node.  
 */

mxml_node_t *				/* O - New node */
stpi_mxmlNewText(mxml_node_t *parent,	/* I - Parent node or MXML_NO_PARENT */
            int         whitespace,	/* I - 1 = leading whitespace, 0 = no whitespace */
	    const char  *string)	/* I - String */
{
  mxml_node_t	*node;			/* New node */


 /*
  * Range check input...
  */

  if (!parent || !string)
    return (NULL);

 /*
  * Create the node and set the text value...
  */

  if ((node = mxml_new(parent, MXML_TEXT)) != NULL)
  {
    node->value.text.whitespace = whitespace;
    node->value.text.string     = strdup(string);
  }

  return (node);
}


/*
 * 'stpi_mxmlRemove()' - Remove a node from its parent.
 *
 * Does not free memory used by the node - use stpi_mxmlDelete() for that.
 * This function does nothing if the node has no parent.
 */

void
stpi_mxmlRemove(mxml_node_t *node)		/* I - Node to remove */
{
 /*
  * Range check input...
  */

/*  fprintf(stderr, "stpi_mxmlRemove(node=%p)\n", node);*/

  if (!node || !node->parent)
    return;

 /*
  * Remove from parent...
  */

  if (node->prev)
    node->prev->next = node->next;
  else
    node->parent->child = node->next;

  if (node->next)
    node->next->prev = node->prev;
  else
    node->parent->last_child = node->prev;

  node->parent = NULL;
  node->prev   = NULL;
  node->next   = NULL;
}


/*
 * 'mxml_new()' - Create a new node.
 */

static mxml_node_t *			/* O - New node */
mxml_new(mxml_node_t *parent,		/* I - Parent node */
         mxml_type_t type)		/* I - Node type */
{
  mxml_node_t	*node;			/* New node */


 /*
  * Allocate memory for the node...
  */

  if ((node = calloc(1, sizeof(mxml_node_t))) == NULL)
    return (NULL);

 /*
  * Set the node type...
  */

  node->type = type;

 /*
  * Add to the parent if present...
  */

  if (parent)
    stpi_mxmlAdd(parent, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, node);

 /*
  * Return the new node...
  */

  return (node);
}


/*
 * End of "$Id: mxml-node.c,v 1.5 2004/04/25 12:17:51 rleigh Exp $".
 */
