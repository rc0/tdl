/*;
   $Header: /cvs/src/tdl/done.c,v 1.11 2003/07/17 22:35:04 richard Exp $
  
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
        char *ident = get_ident(y);
        fprintf(stderr, "Cannot mark %s done, it has open sub-tasks\n", ident);
        free(ident);
      } else {
        y->done = done_time;
      }
    }
  }
}
/*}}}*/
static int internal_done(time_t when, char **x)/*{{{*/
{
  int do_descendents;
  struct nodelist *nl = make_nodelist(), *a;

  clear_flags(&top);

  while (*x) {
    do_descendents = include_descendents(*x); /* May modify *x */
    lookup_nodes(*x, nl, do_descendents);
    x++;
  }

  for (a=nl->next; a!=nl; a=a->next) {
    a->node->flag = 1;
    if (a->flag) {
      mark_all_descendents(a->node);
    }
  }

  free_nodelist(nl);
   
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
  struct nodelist *nl, *a;
  int do_descendents;

  while (*x) {
    nl = make_nodelist();
    do_descendents = include_descendents(*x); /* May modify *x */
    lookup_nodes(*x, nl, 0);
    for(a=nl->next; a!=nl; a=a->next) {
      a->node->done = 0;
      undo_ancestors(a->node);
      if (do_descendents) {
        undo_descendents(a->node);
      }
    }
    free_nodelist(nl);
    x++;
  }

  return 0;

}
/*}}}*/

