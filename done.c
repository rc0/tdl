/*
   $Header: /cvs/src/tdl/done.c,v 1.3 2001/08/23 21:23:31 richard Exp $
  
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
static void mark_done_from_bottom_up(struct links *x)/*{{{*/
{
  struct node *y; 

  for (y = x->next; y != (struct node *) x; y = y->chain.next) {

    if (has_kids(y)) {
      mark_done_from_bottom_up(&y->kids);
    }
    
    if (y->flag) {
      if (has_open_child(y)) {
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
    n = lookup_node(*x, 0, NULL);
    n->flag = 1;
    n->scratch = *x; /* Safe to alias, *x has long lifetime */
    x++;
  }
   
  mark_done_from_bottom_up(&top);

}
/*}}}*/

