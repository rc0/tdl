/*
   $Header: /cvs/src/tdl/move.c,v 1.7 2003/03/10 00:35:14 richard Exp $
  
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


static int is_ancestor(struct node *anc, struct node *dec)/*{{{*/
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
/*}}}*/

static int process_move_internal(char **x, int below_not_above, int into_parent)/*{{{*/
{
  /* x is the argument list
   * below_not_above is true to insert below x[0], false to insert above
   * into_parent means an implicit .0 is appended to x[0] to get the path
   */
  
  int argc, i, n;
  struct links *insert_point;
  struct node **table;
  struct node *insert_parent, *insert_peer, *parent;
  
  char *option;

  option = below_not_above ? "below" : "above";
  
  argc = count_args(x);
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <index_to_insert_%s> <index_to_move> ...\n",
            option, option);
    return -1;
  }

  n = argc - 1;
  if (into_parent) {
    insert_parent = lookup_node(x[0], 0, NULL);
    if (!insert_parent) return -1;
    insert_point = &insert_parent->kids;
  } else {
    insert_peer = lookup_node(x[0], 1, &insert_parent); /* Allow final component to be zero */
    insert_point = (struct links *) &insert_peer->chain;
    if (!insert_point) return -1;
  }
  table = new_array(struct node *, n);
  x++;

  /* Have to do the move in 2 passes, otherwise the indices of the entries
     could change in mid-lookup. */
  for (i=0; i<n; i++) {
    table[i] = lookup_node(x[i], 0, NULL); /* Don't allow zero for this */
    if (!table[i]) return -1; /* memory leak */

    /* Check for an attempt to move a node onto one of its own descendents */
    if (is_ancestor(table[i], insert_parent)) {
      fprintf(stderr, "Can't re-parent entry %s onto a descendent of itself\n", x[i]);
      return -1;
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
    if (into_parent) {
      table[i]->parent = insert_parent;
    } else {
      /* in this case 'insert_peer' is just the one we're putting things above
       * or below, i.e. the entries being moved will be siblings of it and will
       * share its parent. */
      table[i]->parent = insert_peer->parent;
    }
      
    /* To insert the nodes in the command line order */
    if (below_not_above) insert_point = &table[i]->chain;
    /* if inserting above something, the insertion point stays fixed */
    
    /* Clear done status of insertion point and its ancestors */
    if (table[i]->done == 0) {
      parent = insert_parent;
      while (parent) {
        parent->done = 0;
        parent = parent->parent;
      }
    }
  }

  return 0;
}
/*}}}*/
int process_above(char **x)/*{{{*/
{
  return process_move_internal(x, 0, 0);
}
/*}}}*/
int process_below(char **x)/*{{{*/
{
  return process_move_internal(x, 1, 0);
}
/*}}}*/
int process_into(char **x)/*{{{*/
{
  return process_move_internal(x, 0, 1);
}
/*}}}*/
