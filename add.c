/*
   $Header: /cvs/src/tdl/add.c,v 1.16 2003/04/14 23:19:25 richard Exp $
  
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

#include <assert.h>
#include <ctype.h>
#include "tdl.h"

static int add_new_node(char *parent_path, int set_done, int set_priority, enum Priority new_priority, time_t insert_time, char *child_text)/*{{{*/
{
  struct node *parent = NULL;
  struct node *nn;

  if (parent_path) {
    parent = lookup_node(parent_path, 0, NULL);
    if (!parent) return -1;
  } else {
    struct node *narrow_top;
    narrow_top = get_narrow_top();
    if (narrow_top) {
      parent = narrow_top;
    } else {
      /* If all else fails, i.e. new node is a top-level one. */
      parent = NULL;
    }
  }
  
  nn = new_node();
  nn->text = new_string(child_text);
  nn->arrived = (long) insert_time;
  if (set_done) {
    nn->done = (long) insert_time;
  }
  
  nn->priority = (parent && !set_priority) ? parent->priority
                                           : new_priority;

  prepend_child(nn, parent);

  /* Clear done status of parents - they can't be any longer! */
  if (!set_done) {
    while (parent) {
      parent->done = 0;
      parent = parent->parent;
    }
  }

  return 0;
}
/*}}}*/
static char *make_composite(char *prefix, char *suffix)/*{{{*/
{
  int plen, slen;
  int tlen;
  char *result;
  int suffix_is_dot;
  
  plen = prefix ? strlen(prefix) : 0;
  slen = suffix ? strlen(suffix) : 0;
  suffix_is_dot = slen ? (!strcmp(suffix,".")) : 0;
  tlen = plen + slen;
  if (plen && slen) ++tlen; /* for internal '.' */
  if (!plen && !slen) return NULL;
  result = new_array(char, 1+tlen);
  result[0] = '\0';
  if (plen) {
    strcat(result, prefix);
    if (slen && !suffix_is_dot) strcat(result, ".");
  }
  if (slen && !suffix_is_dot) strcat(result, suffix);
  return result;
}
/*}}}*/
static int try_add_interactive(char *parent_path, int set_done)/*{{{*/
{
  char *text;
  time_t insert_time;
  int error = 0;
  int blank;
  int status;
  char prompt[128];
  char *prompt_base;
  char *composite_index;

  composite_index = make_composite(get_narrow_prefix(), parent_path);

  prompt_base = set_done ? "log" : "add";
  if (composite_index) {
    sprintf(prompt, "%s (%s)> ", prompt_base, composite_index);
  } else {
    sprintf(prompt, "%s> ", prompt_base);
  }
  if (composite_index) free(composite_index);
  
  do {
    text = interactive_text(prompt, NULL, &blank, &error);
    if (error < 0) return error;
    if (!blank) {
      time(&insert_time);
      status = add_new_node(parent_path, set_done, 0, PRI_NORMAL, insert_time, text);
      free(text);
      if (status < 0) return status;
    }
  } while (!blank);
  return 0;
}
/*}}}*/
static int could_be_index(char *x)/*{{{*/
{
  enum {AFTER_DIGIT, AFTER_PERIOD, AFTER_MINUS} state;
  char *p;
  state = AFTER_PERIOD;
  for (p=x; *p; p++) {
    switch (state) {
      case AFTER_DIGIT:
        if (isdigit(*p))    state = AFTER_DIGIT;
        else if (*p == '-') state = AFTER_MINUS;
        else if (*p == '.') state = AFTER_PERIOD;
        else return 0;
        break;
      case AFTER_PERIOD:
        if (isdigit(*p))    state = AFTER_DIGIT;
        else if (*p == '-') state = AFTER_MINUS;
        else return 0;
        break;
      case AFTER_MINUS:
        if (isdigit(*p))    state = AFTER_DIGIT;
        else return 0;
        break;
    }
  }
  return 1;
}
/*}}}*/
static int process_add_internal(char **x, int set_done)/*{{{*/
{
  /* Expect 1 argument, the string to add.  Need other options at some point. */
  time_t insert_time;
  int argc = count_args(x);
  char *text = NULL;
  char *parent_path = NULL;
  enum Priority priority = PRI_NORMAL;
  int set_priority = 0;
  char *x0;
  int error;

  insert_time = time(NULL);

  if ((argc > 1) && (x[0][0] == '@')) {
    int error;
    /* For 'add', want date to be in the future.
     * For 'log', want date to be in the past, as for 'done' */
    int default_positive = set_done ? 0 : 1;
    insert_time = parse_date(x[0]+1, insert_time, default_positive, &error);
    if (error < 0) return error;
    argc--;
    x++;
  }
  
  switch (argc) {
    case 0:
      return try_add_interactive(NULL, set_done);
      break;
    case 1:
      if (could_be_index(x[0])) {
        return try_add_interactive(x[0], set_done);
      } else {
        text = x[0];
      }
      break;
    case 2:
      text = x[1];
      x0 = x[0];
      if (isdigit(x0[0]) || (x0[0] == '-') || (x0[0] == '+')) {
        parent_path = x[0];
      } else {
        priority = parse_priority(x0, &error);
        if (error < 0) return error;
        set_priority = 1;
      }
      break;
    case 3:
      text = x[2];
      priority = parse_priority(x[1], &error);
      if (error < 0) return error;
      set_priority = 1;
      parent_path = x[0];
      break;

    default:
      fprintf(stderr, "Usage : add [@<datespec>] [<parent_index>] [<priority>] <entry_text>\n");
      return -1;
      break;
  }

  return add_new_node(parent_path, set_done, set_priority, priority, insert_time, text);

  return 0;
}
/*}}}*/
int process_add(char **x)/*{{{*/
{
  return process_add_internal(x, 0);
}
/*}}}*/
int process_log(char **x)/*{{{*/
{
  return process_add_internal(x, 1);
}
/*}}}*/
static void modify_tree_arrival_time(struct node *y, time_t new_time)/*{{{*/
{
  struct node *c;
  for (c = y->kids.next; c != (struct node *) &y->kids; c = c->chain.next) {
    c->arrived = new_time;
    if (has_kids(c)) {
      modify_tree_arrival_time(c, new_time);
    }
  }
}
/*}}}*/
static int internal_postpone_open(char **x, time_t when)/*{{{*/
{
  struct node *n;
  int do_descendents;
  
  while (*x) {
    do_descendents = include_descendents(*x); /* May modify *x */
    n = lookup_node(*x, 0, NULL);
    if (n) {
      n->arrived = when;
      if (do_descendents) modify_tree_arrival_time(n, when);
    }
    x++;
  }
  return 0;
}
/*}}}*/
int process_postpone(char **x)/*{{{*/
{
  return internal_postpone_open(x, POSTPONED_TIME);
}
/*}}}*/
int process_open(char **x)/*{{{*/
{
  return internal_postpone_open(x, time(NULL));
}
/*}}}*/
int process_defer(char **x)/*{{{*/
{
  int argc;
  time_t new_start_time;
  int error;
  char *date_start;

  argc = count_args(x);
  if (argc < 2) {
    fprintf(stderr, "Usage: defer <datespec> <index>...\n");
    return -1;
  }

  date_start = x[0];
  new_start_time = time(NULL);
  /* 'defer' always takes a date so the @ is optional. */
  if (*date_start == '@') date_start++;
  new_start_time = parse_date(date_start, new_start_time, 1, &error);
  return internal_postpone_open(x+1, new_start_time);
}
/*}}}*/
int process_edit(char **x) /*{{{*/
{
  int argc;
  struct node *n;

  argc = count_args(x);

  if ((argc < 1) || (argc > 2)) {
    fprintf(stderr, "Usage: edit <path> [<new_text>]\n");
    return -1;
  }

  n = lookup_node(x[0], 0, NULL);
  if (!n) return -1;
  if (argc == 2) {
    free(n->text);
    n->text = new_string(x[1]);
  } else {
    int error;
    int is_blank;
    char *new_text;
    char prompt[128];
    char *composite_index;

    composite_index = make_composite(get_narrow_prefix(), x[0]);
    assert(composite_index);
    sprintf(prompt, "edit (%s)> ", composite_index);
    new_text = interactive_text(prompt, n->text, &is_blank, &error);
    free(composite_index);
    if (error < 0) return error;
    if (!is_blank) {
      free(n->text);
      n->text = new_text; /* We own the pointer now */
    }
  }

  return 0;
}
/*}}}*/
