/*
   $Header: /cvs/src/tdl/narrow.c,v 1.1 2003/03/10 22:00:50 richard Exp $
  
   tdl - A console program for managing to-do lists
   Copyright (C) 2001-2003  Richard P. Curnow

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

static struct node *narrow_top = NULL;
static char *narrow_prefix = NULL;

struct node *get_narrow_top(void)/*{{{*/
{
  return narrow_top;
}
/*}}}*/
char *get_narrow_prefix(void)/*{{{*/
{
  return narrow_prefix;
}
/*}}}*/
static char *generate_prefix(struct node *x, char *suffix)
{
  int count;
  struct node *y, *parent;
  char buffer[16];
  int suffix_len, count_len;
  int total_len;
  char *result;
  char *new_suffix;

  parent = x->parent;
  count = 1;
  if (parent) {
    for (y = parent->kids.next; y != x; y = y->chain.next) count++;
  } else {
    for (y = top.next; y != x; y = y->chain.next) count++;
  }
  sprintf(buffer, "%d", count);
  count_len = strlen(buffer);
  suffix_len = strlen(suffix);
  total_len = count_len + suffix_len;
  if (suffix_len) ++total_len; /* allow for '.' */
  
  new_suffix = new_array(char, 1+total_len);
  strcpy(new_suffix, buffer);
  if (suffix_len) strcat(new_suffix, ".");
  strcat(new_suffix, suffix);

  if (parent) {
    result = generate_prefix(parent, new_suffix);
    free(new_suffix);
    return result;
  } else {
    return new_suffix;
  }
}

static void setup_narrow_prefix(void)/*{{{*/
{
  if (narrow_prefix) {
    free(narrow_prefix);
    narrow_prefix = NULL;
  }

  narrow_prefix = generate_prefix(narrow_top, "");
}
/*}}}*/
int process_narrow(char **x)/*{{{*/
{
  int argc;
  struct node *n;

  argc = count_args(x);

  if (argc < 1) {
    fprintf(stderr, "Usage : narrow <entry_index>\n");
    return -1;
  }

  n = lookup_node(x[0], 0, NULL);
  if (!n) return -1;

  narrow_top = n;

  /* Compute prefix string. */
  setup_narrow_prefix();

  return 0;
}
/*}}}*/
int process_widen(char **x)/*{{{*/
{
  int argc;
  int n_levels;
  int i;
  struct node *new_narrow_top;
  
  argc = count_args(x);
  if (argc > 1) {
    fprintf(stderr, "Usage : widen [<n_levels>]\n");
    return -1;
  }
  if (argc > 0) {
    if (sscanf(x[0], "%d", &n_levels) != 1) {
      fprintf(stderr, "Usage : widen [<n_levels>]\n");
      return -1;
    }
  } else {
    n_levels = 1;
  }

  new_narrow_top = narrow_top;
  for (i=0; i<n_levels; i++) {
    new_narrow_top = new_narrow_top->parent;
    if (!new_narrow_top) goto widen_to_top;
  }

  narrow_top = new_narrow_top;
  setup_narrow_prefix();
  return 0;

    /* Widen back to top level */
widen_to_top:
  narrow_top = NULL;
  if (narrow_prefix) {
    free(narrow_prefix);
    narrow_prefix = NULL;
  }

  return 0;
}
/*}}}*/

