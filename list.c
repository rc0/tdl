/*
   $Header: /cvs/src/tdl/list.c,v 1.1 2001/08/20 22:38:00 richard Exp $
  
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
#include <string.h>
#include "tdl.h"

/*{{{ Colour definitions */
#define RED     "[31m[1m"
#define GREEN   "[32m"
#define YELLOW  "[33m[1m"
#define BLUE    "[34m"
#define MAGENTA "[35m"
#define CYAN    "[36m"
#define NORMAL  "[0m"
#define DIM     "[37m[2m"

/* Table to map priority levels to colours */
static char *colour_table[] = {
  NORMAL, BLUE, CYAN, NORMAL, YELLOW, RED
};

static char *priority_text[] = {
  "UNKNOWN!", "verylow", "low", "normal", "high", "urgent"
};

/*}}}*/
void do_indent(int indent)/*{{{*/
{
  int i;
  for (i=0; i<indent; i++) putchar(' ');
}
/*}}}*/
void do_bullet_indent(int indent)/*{{{*/
{
  int i;
  int n;
  n = indent - 2;
  for (i=0; i<indent; i++) putchar((i == n) ? '-' : ' ');
}
/*}}}*/
static void print_timestamp(int timestamp, char *leader, int indent)/*{{{*/
{
  char buffer[32];
  time_t now;
  long diff, days_ago;
  
  now = time(NULL);
  diff = now - timestamp;
  days_ago = diff / 86400;
  strftime(buffer, sizeof(buffer), "%a %d %b %Y %H:%M", 
           localtime((time_t *)&timestamp));
  do_indent(indent+2);
  printf("%s%s:%s %s (%ld day%s ago)\n", GREEN, leader, NORMAL, buffer, days_ago, (days_ago == 1) ? "" : "s");
}
/*}}}*/
static void list_chain(struct links *x, int indent, int verbose, int all, char *index_buffer)/*{{{*/
{
  struct node *y;
  int idx, is_done;
  char component_buffer[8];
  char new_index_buffer[64];
  char *p;
  
  for (y = x->next, idx = 1;
       y != (struct node *) x;
       y = y->chain.next, ++idx) {
    
    is_done = (y->done > 0);
    if (!all && is_done) continue;

    do_indent(indent);
    sprintf(component_buffer, "%d", idx);
    strcpy(new_index_buffer, index_buffer);
    if (strlen(new_index_buffer) > 0) {
      strcat(new_index_buffer, ".");
    }
    strcat(new_index_buffer, component_buffer);
    printf("%s%s%s %s%s", GREEN, new_index_buffer, NORMAL,
           (all && !verbose && is_done) ? CYAN "(DONE) " NORMAL : "",
           is_done ? CYAN : colour_table[y->priority]);
    for (p = y->text; *p; p++) {
      putchar(*p);
      if (*p == '\n') {
        do_indent(indent + 5);
      }
    }
    printf("%s\n", NORMAL);
    if (verbose) {
      print_timestamp(y->arrived, "Arrived", indent);
      do_indent(indent + 2);
      printf("%sPriority: %s%s%s\n",
             GREEN, colour_table[y->priority], priority_text[y->priority], NORMAL);
      if (y->required_by > 0) print_timestamp(y->required_by, "Required by", indent);
      if (y->done > 0) print_timestamp(y->done, "Completed", indent);
      printf("\n");
    }

    list_chain(&y->kids, indent + 3, verbose, all, new_index_buffer);
  }
  return;
}
/*}}}*/
void process_list(char **x)/*{{{*/
{
  int verbose = 0;
  int all = 0; /* Even show done nodes */
  char index_buffer[2];

  while (*x) {
    if (!strcmp(*x, "-v")) verbose = 1;
    if (!strcmp(*x, "-a")) all = 1;
    x++;
  }
  
  index_buffer[0] = 0;

  list_chain(&top, 0, verbose, all, index_buffer);
}
/*}}}*/
