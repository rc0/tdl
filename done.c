/*
   $Header: /cvs/src/tdl/done.c,v 1.1 2001/08/20 22:38:00 richard Exp $
  
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

static void mark_done_from_bottom_up(struct links *x)/*{{{*/
{
  struct node *y, *c; 
  int has_open_child;

  for (y = x->next; y != (struct node *) x; y = y->chain.next) {

    if (has_kids(y)) {
      mark_done_from_bottom_up(&y->kids);
    }
    
    if (y->flag) {
      has_open_child = 0;
      if (has_kids(y)) {
        for (c = y->kids.next; c != (struct node *) &y->kids; c = c->chain.next) {
          if (c->done == 0) {
            has_open_child = 1;
            break;
          }
        }
      }
      if (has_open_child) {
        fprintf(stderr, "Cannot mark %s done, it has open sub-tasks\n", y->scratch);
      } else {
        time_t now;
        now = time(NULL);
        y->done = now;
      }
    }
  }
}
/*}}}*/
void process_done(char **x)/*{{{*/
{
  struct node *n;

  clear_flags(&top);

  while (*x) {
    n = lookup_node(*x);
    n->flag = 1;
    n->scratch = *x; /* Safe to alias, *x has long lifetime */
    x++;
  }
   
  mark_done_from_bottom_up(&top);

}
/*}}}*/

