/*
   $Header: /cvs/src/tdl/report.c,v 1.1 2001/08/20 22:38:00 richard Exp $
  
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
      for (p = y->text; *p; p++) {
        putchar(*p);
        if (*p == '\n') {
          do_indent(indent + 2);
        }
      }
      putchar('\n');

      if (has_kids(y)) {
        print_report(&y->kids, indent + 3);
      }
    }
  }
}
/*}}}*/
long read_interval(char *xx)/*{{{*/
{
  long interval;
  int nc;
  
  if (sscanf(xx, "%ld%n", &interval, &nc) != 1) {
    fprintf(stderr, "Cannot recognize interval '%s'\n", xx);
    exit(1);
  }
  
  xx += nc;
  switch (xx[0]) {
    case 'h': interval *= 3600; break;
    case 'd': interval *= 86400; break;
    case 'w': interval *= 7 * 86400; break;
    case 'm': interval *= 30 * 86400; break;
    case 'y': interval *= 365 * 86400; break;
    case 's':
    case '\0': break; /* no multiplier or 's' : use seconds */
    default:
      fprintf(stderr, "Can't understand interval multiplier '%s'\n", xx);
      exit(1);
  }
  return interval;
}
/*}}}*/
void process_report(char **x)/*{{{*/
{
  long interval;
  time_t now, start, end;
  int argc;

  argc = count_args(x);
  start = end = now = time(NULL);

  switch (argc) {
    case 1:
      interval = read_interval(x[0]);
      start = now - interval;
      break;
    case 2:
      interval = read_interval(x[0]);
      start = now - interval;
      interval = read_interval(x[1]);
      end = now - interval;
      break;
    default:
      fprintf(stderr, "Usage: report <start_ago> [<end_ago>] (e.g. 3h, 5d, 1w etc)\n");
      break;
  }

  clear_flags(&top);
  mark_nodes_done_since(&top, start, end);
  print_report(&top, 0);
}
/*}}}*/
