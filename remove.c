/*
   $Header: /cvs/src/tdl/remove.c,v 1.2 2001/08/21 22:43:24 richard Exp $
  
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

#include <string.h>
#include "tdl.h"

static void delete_from_bottom_up(struct links *x)/*{{{*/
{
  struct node *y, *ny; 
  for (y = x->next; y != (struct node *) x; y = ny) {

    /* Calculate this now in case y gets deleted later */
    ny = y->chain.next; 
    
    if (has_kids(y)) {
      delete_from_bottom_up(&y->kids);
    }

    if (y->flag) {
      if (has_kids(y)) {
        fprintf(stderr, "Cannot delete %s, it has sub-tasks\n", y->scratch);
      } else {
        /* Just unlink from the chain for now, don't bother about full clean up. */
        struct node *next, *prev;
        next = y->chain.next;
        prev = y->chain.prev;
        prev->chain.next = next;
        next->chain.prev = prev;
      }
    }
  }
}
/*}}}*/
void process_remove(char **x)/*{{{*/
{
  struct node *n;

  clear_flags(&top);

  while (*x) {
    n = lookup_node(*x, 0, NULL);
    n->flag = 1;
    n->scratch = *x; /* *x has long lifetime => OK to alias */
    ++x;
  }

  delete_from_bottom_up(&top);
  
}
/*}}}*/
