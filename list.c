/*
   $Header: /cvs/src/tdl/list.c,v 1.5 2001/10/07 22:44:46 richard Exp $
  
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
#include <ctype.h>
#include "tdl.h"

#define INDENT_TAB 3

/*{{{ Colour definitions */
#define RED     "[31m[1m"
#define GREEN   "[32m"
#define YELLOW  "[33m[1m"
#define BLUE    "[34m"
#define MAGENTA "[35m"
#define CYAN    "[36m"
#define NORMAL  "[0m"
#define DIM     "[37m[2m"
#define DIMCYAN "[36m[2m"

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
static void print_timestamp(int timestamp, char *leader, int indent, int monochrome)/*{{{*/
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
  if (monochrome) {
    printf("%s: %s (%ld day%s ago)\n", leader, buffer, days_ago, (days_ago == 1) ? "" : "s");
  } else {
    printf("%s%s:%s %s (%ld day%s ago)\n", GREEN, leader, NORMAL, buffer, days_ago, (days_ago == 1) ? "" : "s");
  }
}
/*}}}*/
static void print_details(struct node *y, int indent, int verbose, int all, int monochrome, char *index_buffer)/*{{{*/
{
  int is_done;
  char *p;

  is_done = (y->done > 0);
  if (!all && is_done) return;

  do_indent(indent);
  if (monochrome) {
    printf("%s %s", index_buffer,
           (all && !verbose && is_done) ? "(DONE) : " : ": ");
  } else {
    printf("%s%s%s %s%s", GREEN, index_buffer, NORMAL,
           (all && !verbose && is_done) ? CYAN "(DONE) " NORMAL : "",
           is_done ? DIMCYAN : colour_table[y->priority]);
  }
  for (p = y->text; *p; p++) {
    putchar(*p);
    if (*p == '\n') {
      do_indent(indent + 5);
    }
  }
  if (!monochrome) printf("%s", NORMAL);
  printf("\n");
  if (verbose) {
    print_timestamp(y->arrived, "Arrived", indent, monochrome);
    do_indent(indent + 2);
    if (monochrome) {
      printf("Priority: %s\n", priority_text[y->priority]);
    } else {
      printf("%sPriority: %s%s%s\n",
          GREEN, colour_table[y->priority], priority_text[y->priority], NORMAL);
    }
    if (y->required_by > 0) print_timestamp(y->required_by, "Required by", indent, monochrome);
    if (y->done > 0) print_timestamp(y->done, "Completed", indent, monochrome);
    printf("\n");
  }

}
/*}}}*/
static void list_chain(struct links *x, int indent, int verbose, int all, int monochrome, char *index_buffer, enum Priority prio)/*{{{*/
{
  struct node *y;
  int idx, is_done;
  char component_buffer[8];
  char new_index_buffer[64];
  
  for (y = x->next, idx = 1;
       y != (struct node *) x;
       y = y->chain.next, ++idx) {
    
    is_done = (y->done > 0);
    if (!all && is_done) continue;

    sprintf(component_buffer, "%d", idx);
    strcpy(new_index_buffer, index_buffer);
    if (strlen(new_index_buffer) > 0) {
      strcat(new_index_buffer, ".");
    }
    strcat(new_index_buffer, component_buffer);

    if (y->priority >= prio) {
      print_details(y, indent, verbose, all, monochrome, new_index_buffer);
    }

    /* Maybe list children regardless of priority assigned to parent. */
    list_chain(&y->kids, indent + INDENT_TAB, verbose, all, monochrome, new_index_buffer, prio);

  }
  return;
}
/*}}}*/
void process_list(char **x)/*{{{*/
{
  int verbose = 0;
  int all = 0; /* Even show done nodes */
  int any_paths = 0;
  int monochrome = 0;
  char index_buffer[64];
  char *y;
  enum Priority prio = PRI_NORMAL;

  if (getenv("TDL_LIST_MONOCHROME") != 0) {
    monochrome = 1;
  }
  
  while ((y = *x) != 0) {
    if (!strcmp(y, "-v")) verbose = 1;
    else if (!strcmp(y, "-a")) all = 1;
    else if (!strcmp(y, "-m")) monochrome = 1;
    else if (isdigit(y[0]) ||
             (((y[0] == '-') || (y[0] == '+')) &&
              (isdigit(y[1])))) {
      
      struct node *n = lookup_node(y, 0, NULL);
      any_paths = 1;
      index_buffer[0] = '\0';
      strcpy(index_buffer, y);
      print_details(n, 0, verbose, all, monochrome, index_buffer);
      list_chain(&n->kids, INDENT_TAB, verbose, all, monochrome, index_buffer, prio);
    } else {
      prio = parse_priority(y);
    }

    x++;
  }
  
  if (!any_paths) {
    index_buffer[0] = 0;
    list_chain(&top, 0, verbose, all, monochrome, index_buffer, prio);
  }
}
/*}}}*/
