/*
   $Header: /cvs/src/tdl/purge.c,v 1.2 2001/08/21 22:43:24 richard Exp $
  
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

#include <time.h>
#include "tdl.h"

static void purge_from_bottom_up(struct links *x)/*{{{*/
{
  struct node *y, *ny; 
  for (y = x->next; y != (struct node *) x; y = ny) {

    /* Calculate this now in case y gets deleted later */
    ny = y->chain.next; 
    
    if (has_kids(y)) {
      purge_from_bottom_up(&y->kids);
    }

    if (y->flag) {
      /* If the node still has children, just keep quite and don't delete it. 
       * The idea of the 'purge' command is just a fast bulk delete of all
       * old stuff under certain tasks, so this is OK */
      if (!has_kids(y)) {
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
static void scan_from_top_down(struct links *x, time_t then)/*{{{*/
{
  struct node *y; 
  for (y = x->next; y != (struct node *) x; y = y->chain.next) {
    if ((y->done > 0) && (y->done < then)) y->flag = 1;
    scan_from_top_down(&y->kids, then);
  }
}
/*}}}*/

void process_purge(char **x)/*{{{*/
{
  int argc;
  long interval;
  time_t now, then;

  argc = count_args(x);
  if (argc < 1) {
    fprintf(stderr, "Usage: purge <interval> <entry_index> ...\n");
    exit(1);
  }

  interval = read_interval(x[0]);
  x++;

  clear_flags(&top);
  now = time(NULL);
  then = now - interval;
 
  if (!*x) {
    /* If no indices given, do whole database */
    scan_from_top_down(&top, then);
  } else {
    while (*x) {
      struct node *n;
      n = lookup_node(*x, 0, NULL);
      if ((n->done > 0) && (n->done < then)) n->flag = 1;
      scan_from_top_down(&n->kids, then);
      x++;
    }
  }

  purge_from_bottom_up(&top);

}
/*}}}*/
