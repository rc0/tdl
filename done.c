/*
   $Header: /cvs/src/tdl/done.c,v 1.5 2001/10/14 22:08:28 richard Exp $
  
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

int has_open_child(struct node *y)/*{{{*/
{
  struct node *c;
  int result = 0;
  for (c = y->kids.next; c != (struct node *) &y->kids; c = c->chain.next) {
    if (c->done == 0) {
      result = 1;
      break;
    }
  }

  return result;
}
/*}}}*/
static void mark_done_from_bottom_up(struct links *x, time_t done_time)/*{{{*/
{
  struct node *y; 

  for (y = x->next; y != (struct node *) x; y = y->chain.next) {

    if (has_kids(y)) {
      mark_done_from_bottom_up(&y->kids, done_time);
    }
    
    if (y->flag) {
      if (has_open_child(y)) {
        fprintf(stderr, "Cannot mark %s done, it has open sub-tasks\n", y->scratch);
      } else {
        y->done = done_time;
      }
    }
  }
}
/*}}}*/
void process_done(char **x)/*{{{*/
{
  struct node *n;
  int do_descendents;
  time_t done_time = time(NULL);

  clear_flags(&top);

  if (x[0][0] == '@') {
    done_time = parse_date(x[0]+1, done_time, 0);
    x++;
  }

  while (*x) {
    do_descendents = include_descendents(*x); /* May modify *x */
    n = lookup_node(*x, 0, NULL);
    n->flag = 1;
    if (do_descendents) {
      mark_all_descendents(n);
    }
    n->scratch = *x; /* Safe to alias, *x has long lifetime */
    x++;
  }
   
  mark_done_from_bottom_up(&top, done_time);

}
/*}}}*/

