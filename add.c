/*
   $Header: /cvs/src/tdl/add.c,v 1.6 2002/05/09 23:07:06 richard Exp $
  
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

#include <ctype.h>
#include "tdl.h"

int process_add(char **x, int set_done)/*{{{*/
{
  /* Expect 1 argument, the string to add.  Need other options at some point. */
  time_t insert_time;
  struct node *nn;
  int argc = count_args(x);
  char *text = NULL;
  char *parent_path = NULL;
  struct node *parent = NULL;
  enum Priority priority = PRI_NORMAL;
  int set_priority = 0;
  char *x0;
  int error;

  insert_time = time(NULL);

  if ((argc > 1) && (x[0][0] == '@')) {
    insert_time = parse_date(x[0]+1, insert_time, 1);
    argc--;
    x++;
  }
  
  switch (argc) {
    case 1:
      text = x[0];
      break;
    case 2:
      text = x[1];
      x0 = x[0];
      if (isdigit(x0[0]) || (x0[0] == '-') || (x0[0] == '+')) {
        parent_path = x[0];
      } else {
        priority = parse_priority(x0, &error);
        if (error < 0) return error;
        set_priority = 1;
      }
      break;
    case 3:
      text = x[2];
      priority = parse_priority(x[1], &error);
      if (error < 0) return error;
      set_priority = 1;
      parent_path = x[0];
      break;

    default:
      fprintf(stderr, "Usage : add [@<datespec>] [<parent_index>] [<priority>] <entry_text>\n");
      return -1;
      break;
  }

  if (parent_path) {
    parent = lookup_node(parent_path, 0, NULL);
    if (!parent) return -1;
  } else {
    parent = NULL;
  }
  
  nn = new_node();
  nn->text = new_string(text);
  nn->arrived = (long) insert_time;
  if (set_done) {
    nn->done = (long) insert_time;
  }
  
  nn->priority = (parent && !set_priority) ? parent->priority
                                           : priority;

  prepend_child(nn, parent);

  /* Clear done status of parents - they can't be any longer! */
  if (!set_done) {
    while (parent) {
      parent->done = 0;
      parent = parent->parent;
    }
  }
  
  return 0;
}
/*}}}*/
static void modify_tree_arrival_time(struct node *y, time_t new_time)/*{{{*/
{
  struct node *c;
  for (c = y->kids.next; c != (struct node *) &y->kids; c = c->chain.next) {
    c->arrived = new_time;
    if (has_kids(c)) {
      modify_tree_arrival_time(c, new_time);
    }
  }
}
/*}}}*/
int process_edit(char **x) /*{{{*/
{
  int argc;
  struct node *n;
  time_t new_insert_time;
  int had_new_time;
  int do_descendents;

  argc = count_args(x);

  new_insert_time = time(NULL);
  if ((argc > 1) && (x[0][0] == '@')) {
    new_insert_time = parse_date(x[0]+1, new_insert_time, 1);
    argc--;
    x++;
    had_new_time = 1;
  } else {
    had_new_time = 0;
  }
  
  if ((argc < 1) || (argc > 2)) {
    fprintf(stderr, "Usage: edit [@<datespec>] <path> [<new_text>]\n");
    return -1;
  }

  do_descendents = include_descendents(*x); /* May modify *x */
  if (do_descendents && (argc == 2)) {
    fprintf(stderr, "You can't use '...' to modify the text for >1 entry at once\n");
    return -1;
  }
  n = lookup_node(x[0], 0, NULL);
  if (!n) return -1;
  if (argc == 2) {
    free(n->text);
    n->text = new_string(x[1]);
  }
  if (had_new_time) {
    n->arrived = new_insert_time;
    if (do_descendents) modify_tree_arrival_time(n, new_insert_time);
  }
  return 0;
}
/*}}}*/
