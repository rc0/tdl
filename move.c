/*
   $Header: /cvs/src/tdl/move.c,v 1.1 2001/08/21 22:43:24 richard Exp $
  
   tdl - A console program for managing to-do lists
   Copyright (C) 2001  Richard P. Curnow

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
   */

#include "tdl.h"


static int is_ancestor(struct node *anc, struct node *dec)
{
  /* Check includes whether the two nodes are the same */
  struct node *parent;
  for (parent = dec; parent != NULL; parent = parent->parent) {
    if (parent == anc) {
      return 1;
    }
  }
  return 0;
}


void process_move(char **x, int below_not_above)
{
  int argc, i, n;
  struct links *insert_point;
  struct node **table;
  struct node *insert_parent, *parent;
  
  char *option;

  option = below_not_above ? "below" : "above";
  
  argc = count_args(x);
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <index_to_insert_%s> <index_to_move> ...\n",
            option, option);
    exit(1);
  }

  n = argc - 1;
  insert_point = (struct links *) lookup_node(x[0], 1, &insert_parent); /* Allow final component to be zero */
  table = new_array(struct node *, n);
  x++;

  /* Have to do the move in 2 passes, otherwise the indices of the entries
     could change in mid-lookup. */
  for (i=0; i<n; i++) {
    table[i] = lookup_node(x[i], 0, NULL); /* Don't allow zero for this */

    /* Check for an attempt to move a node onto one of its own descendents */
    if (is_ancestor(table[i], insert_parent)) {
      fprintf(stderr, "Can't re-parent entry %s onto a descendent of itself\n", x[i]);
      exit(1);
    }
  }

  for (i=0; i<n; i++) { 
    /* Unlink from its current location */
    struct node *prev, *next;
    next = table[i]->chain.next;
    prev = table[i]->chain.prev;
    prev->chain.next = next;
    next->chain.prev = prev;

    (below_not_above ? append_node : prepend_node) (table[i], insert_point);
    /* To insert the nodes in the command line order */
    if (below_not_above) insert_point = &table[i]->chain;
    /* if inserting above something, the insertion point stays fixed */
    
  }

  /* Clear done status of insertion point and its ancestors */
  parent = insert_parent;
  while (parent) {
    parent->done = 0;
    parent = parent->parent;
  }

}

