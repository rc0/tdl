/*;
   $Header: /cvs/src/tdl/done.c,v 1.10.2.1 2004/01/07 00:09:05 richard Exp $
  
   tdl - A console program for managing to-do lists
   Copyright (C) 2001-2004  Richard P. Curnow

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
static int internal_done(time_t when, char **x)/*{{{*/
{
  struct node *n;
  int do_descendents;

  clear_flags(&top);

  while (*x) {
    do_descendents = include_descendents(*x); /* May modify *x */
    n = lookup_node(*x, 0, NULL);
    if (!n) return -1;
    n->flag = 1;
    if (do_descendents) {
      mark_all_descendents(n);
    }
    n->scratch = *x; /* Safe to alias, *x has long lifetime */
    x++;
  }
   
  mark_done_from_bottom_up(&top, when);

  return 0;
}
/*}}}*/
int process_done(char **x)/*{{{*/
{
  time_t done_time = time(NULL);

  if (*x && (x[0][0] == '@')) {
    int error;
    done_time = parse_date(x[0]+1, done_time, 0, &error);
    if (error < 0) return error;
    x++;
  }

  internal_done(done_time, x);
  return 0;
}
/*}}}*/
int process_ignore(char **x)/*{{{*/
{
  /* (struct node).done == 0 means not done yet.  So use 1 to mean ancient
   * history (never shows up in reports, sure to be purged at next purge) */
  internal_done(IGNORED_TIME, x);
  return 0;
}
/*}}}*/
static void undo_descendents(struct node *y)/*{{{*/
{
  struct node *c;
  for (c = y->kids.next; c != (struct node *) &y->kids; c = c->chain.next) {
    c->done = 0;
    if (has_kids(c)) {
      undo_descendents(c);
    }
  }
}
/*}}}*/
static void undo_ancestors(struct node *y)/*{{{*/
{
  struct node *parent;
  parent = y->parent;
  while (parent) {
    parent->done = 0;
    parent = parent->parent;
  }
}
/*}}}*/
int process_undo(char **x)/*{{{*/
{
  struct node *n;
  int do_descendents;

  while (*x) {
    do_descendents = include_descendents(*x); /* May modify *x */
    n = lookup_node(*x, 0, NULL);
    if (!n) return -1;
    n->done = 0;
    undo_ancestors(n);
    if (do_descendents) {
      undo_descendents(n);
    }
    x++;
  }

  return 0;

}
/*}}}*/

