/*
   $Header: /cvs/src/tdl/report.c,v 1.3 2002/04/29 21:38:16 richard Exp $
  
   tdl - A console program for managing to-do lists
   Copyright (C) 2001,2002  Richard P. Curnow

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

static int mark_nodes_done_since(struct links *x, time_t start, time_t end)/*{{{*/
{
  int flag_set = 0;
  struct node *y;

  for (y = x->next; y != (struct node *) x; y = y->chain.next) {
    if ((y->done >= start) && (y->done <= end)) {
      y->flag = 1;
      flag_set = 1;
    }

    if (has_kids(y)) {
      if (mark_nodes_done_since(&y->kids, start, end)) {
        y->flag = 1;
        flag_set = 1;
      }
    }
  }
  return flag_set;
}
/*}}}*/
static void print_report(struct links *x, int indent)/*{{{*/
{
  struct node *y;
  char *p;

  for (y = x->next; y != (struct node *) x; y = y->chain.next) {
    if (y->flag) {
      do_bullet_indent(indent+2);
      if (!y->done) printf ("[[");
      for (p = y->text; *p; p++) {
        putchar(*p);
        if (*p == '\n') {
          do_indent(indent + 2);
        }
      }
      if (!y->done) printf ("]]");
      putchar('\n');

      if (has_kids(y)) {
        print_report(&y->kids, indent + 3);
      }
    }
  }
}
/*}}}*/
void process_report(char **x)/*{{{*/
{
  time_t now, start, end;
  int argc;
  char *d0, *d1;

  argc = count_args(x);
  start = end = now = time(NULL);

  switch (argc) {
    case 1:
      d0 = x[0];
      if (*d0 == '@') d0++;
      start = parse_date(d0, now, 0);
      break;
    case 2:
      d0 = x[0];
      d1 = x[1];
      if (*d0 == '@') d0++;
      if (*d1 == '@') d1++;
      start = parse_date(d0, now, 0);
      end = parse_date(d1, now, 0);
      break;
    default:
      fprintf(stderr, "Usage: report <start_epoch> [<end_epoch>]\n");
      break;
  }

  clear_flags(&top);
  mark_nodes_done_since(&top, start, end);
  print_report(&top, 0);
}
/*}}}*/
