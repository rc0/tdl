/*
   $Header: /cvs/src/tdl/add.c,v 1.1 2001/08/20 22:38:00 richard Exp $
  
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

void process_add(char **x, int set_done)/*{{{*/
{
  /* Expect 1 argument, the string to add.  Need other options at some point. */
  time_t now;
  struct node *nn;
  int argc = count_args(x);
  char *text = NULL;
  char *parent_path = NULL;
  struct node *parent = NULL;
  enum Priority priority = PRI_NORMAL;
  int set_priority = 0;
  char *x0;

  switch (argc) {
    case 1:
      text = x[0];
      break;
    case 2:
      text = x[1];
      x0 = x[0];
      if (isdigit(x0[0])) {
        parent_path = x[0];
      } else {
        priority = parse_priority(x0);
        set_priority = 1;
      }
      break;
    case 3:
      text = x[2];
      priority = parse_priority(x[1]);
      set_priority = 1;
      parent_path = x[0];
      break;

    default:
      fprintf(stderr, "add requires 1, 2 or 3 arguments\n");
      exit(1);
      break;
  }

  if (parent_path) {
    parent = lookup_node(parent_path);
  } else {
    parent = NULL;
  }
  
  now = time(NULL);
  nn = new_node();
  nn->text = new_string(text);
  nn->arrived = (long) now;
  if (set_done) {
    nn->done = (long) now;
  }
  
  nn->priority = (parent && !set_priority) ? parent->priority
                                           : priority;

  append_child(nn, parent);

  /* Clear done status of parents - they can't be any longer! */
  while (parent) {
    parent->done = 0;
    parent = parent->parent;
  }
  
}
/*}}}*/
void process_edit(char **x) /*{{{*/
{
  int argc;
  struct node *n;

  argc = count_args(x);
  if (argc != 2) {
    fprintf(stderr, "Usage: edit <path> <new_text>\n");
    exit(1);
  }

  n = lookup_node(x[0]);
  free(n->text);
  n->text = new_string(x[1]);
  return;
}
/*}}}*/
