/*
   $Header: /cvs/src/tdl/impexp.c,v 1.5 2003/03/10 00:35:14 richard Exp $
  
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

/* This code deals with the import and export commands. */

#include "tdl.h"

static struct node *clone_node(struct node *x, struct node *parent)/*{{{*/
{
  struct node *r;
  struct node *k;
  r = new_node();
  r->text = new_string(x->text);
  r->parent = parent;
  r->priority = x->priority;
  r->arrived = x->arrived;
  r->required_by = x->required_by;
  r->done = x->done;

  for (k = x->kids.next; k != (struct node *) &x->kids; k = k->chain.next) {
    struct node *nc = clone_node(k, r);
    prepend_node(nc, &r->kids);
  }

  return r;
}
/*}}}*/
static void set_arrived_time(struct node *x, time_t now)/*{{{*/
{
  struct node *k;
  x->arrived = now;
  for (k = x->kids.next; k != (struct node *) &x->kids; k = k->chain.next) {
    set_arrived_time(k, now);
  }
}
/*}}}*/
int process_export(char **x)/*{{{*/
{
  FILE *out;
  char *filename;
  int argc, i;
  struct links data;

  argc = count_args(x);

  if (argc < 3) {
    fprintf(stderr, "Need a filename and at least one index to write\n");
    return -2;
  }
  
  filename = x[0];

  data.prev = data.next = (struct node *) &data;

  /* Build linked list of data to write */
  for (i=1; i<argc; i++) {
    struct node *n, *nn;
    n = lookup_node(x[i], 0, NULL);
    if (!n) return -1;
    nn = clone_node(n, NULL);
    prepend_node(nn, &data);
  }
  
  out = fopen(filename, "wb");
  if (!out) {
    fprintf(stderr, "Could not open file %s for writing\n", filename);
    return -2;
  }

  write_database(out, &data);
    
  fclose(out);
  free_database(&data);
  return 0;

}
/*}}}*/
int process_import(char **x)/*{{{*/
{
  char *filename;
  FILE *in;
  struct links data;
  struct node *n, *nn;
  int argc;
  int result;

  argc = count_args(x);
  if (argc < 1) {
    fprintf(stderr, "Import requires a filename\n");
    return -2;
  }

  filename = x[0];
  in = fopen(filename, "rb");
  if (!in) {
    fprintf(stderr, "Could not open file %s for input\n", filename);
    return -2;
  }

  result = read_database(in, &data);
  fclose(in);

  if (!result) {
    /* read was OK */
    for (n = data.next; n != (struct node *) &data; n = nn) {
      nn = n->chain.next;
      prepend_node(n, &top);
      n->parent = NULL;
    }
  }

  return result;

}
/*}}}*/
static int internal_copy_clone(struct links *l, char **x)/*{{{*/
{
  int argc, i;
  time_t now;

  now = time(NULL);

  argc = count_args(x);
  if (argc < 1) {
    fprintf(stderr, "Need at least one index to copy/clone\n");
    return -2;
  }

  for (i=0; i<argc; i++) {
    struct node *n, *nn;
    n = lookup_node(x[i], 0, NULL);
    if (!n) return -1;
    nn = clone_node(n, NULL);
    set_arrived_time(nn, now);
    prepend_node(nn, l);
  }

  return 0;
}
/*}}}*/
int process_clone(char **x)/*{{{*/
{
  return internal_copy_clone(&top, x);
}
/*}}}*/
int process_copyto(char **x)/*{{{*/
{
  struct node *parent;
  if (count_args(x) < 1) {
    fprintf(stderr, "Need a parent index to copy into\n");
    return -2;
  }
  parent = lookup_node(x[0], 0, NULL);
  if (!parent) {
    return -1;
  }
  return internal_copy_clone(&parent->kids, x + 1);
}
/*}}}*/
