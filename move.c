/*
   $Header: /cvs/src/tdl/move.c,v 1.10 2003/08/06 23:23:00 richard Exp $
  
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
static int naming_collision(struct node *mover, struct links *insert_point)/*{{{*/
{
  struct node *y;
  if (mover->name) {
    for (y = insert_point->next; y != (struct node *) insert_point; y = y->chain.next) {
      if (y->name && !strcmp(y->name, mover->name)) {
        return 1;
      }
    }
    return 0;
  } else {
    return 0;
  }
}
/*}}}*/
static void move_node(struct node *a, struct links **insert_point, /*{{{*/
                      struct node *insert_parent, int below_not_above,
                      int do_descendents)
{
  struct node *next, *prev;
  char *old_id, *new_id;
  old_id = get_ident(a);
  next = a->chain.next;
  prev = a->chain.prev;
  prev->chain.next = next;
  next->chain.prev = prev;
  (below_not_above ? append_node : prepend_node) (a, *insert_point);
  a->parent = insert_parent;
  new_id = get_ident(a);
  if (below_not_above) *insert_point = &a->chain;

  printf("moved %s to %s\n", old_id, new_id);
  free(old_id);
  free(new_id);
  
  if (a->done == 0) {
    struct node *parent;
    parent = insert_parent;
    while (parent) {
      parent->done = 0;
      parent = parent->parent;
    }
  }

  if (do_descendents) {
    struct node *y, *next_y;
    for (y = a->kids.next; y != (struct node *) &a->kids; y = next_y) {
      next_y = y->chain.next;
      move_node(y, insert_point, insert_parent, below_not_above, 1);
    }
  }
}
/*}}}*/
static int process_move_internal(char **x, int below_not_above, int into_parent)/*{{{*/
{
  /* x is the argument list
   * below_not_above is true to insert below x[0], false to insert above
   * into_parent means an implicit .0 is appended to x[0] to get the path
   */
  
  int argc, n;
  struct links *insert_point;
  struct node *insert_parent, *insert_peer=NULL;
  struct nodelist *nl, *a;
  
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

  /* Build list of nodes from argument list.  Do all lookups in a first pass,
     o/w the later lookups will match different nodes because of moves caused
     by earlier ones */
  x++;
  nl = make_nodelist();
  while (*x) {
    /* Moving descendents doesn't make sense?  It would just flatten the
       structure out */
    int do_descendents;
    do_descendents = include_descendents(*x);
    lookup_nodes(*x, nl, do_descendents);
    /* Process each node */
    x++;
  }

  for (a = nl->next; a!=nl; a=a->next) {
    if (is_ancestor(a->node, insert_parent)) {
      char *ident = get_ident(a->node);
      fprintf(stderr, "Can't re-parent entry %s onto a descendent of itself\n", ident);
      free(ident);
    } else if (naming_collision(a->node, insert_point)) {
      char *ident = get_ident(a->node);
      fprintf(stderr, "Can't re-parent entry %s : it's name will clash with another child of the new parent\n", ident);
      free(ident);
    } else {
      move_node(a->node, &insert_point, 
                (into_parent ? insert_parent : insert_peer->parent),
                below_not_above, a->flag);
    }
  }

  free_nodelist(nl);

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
