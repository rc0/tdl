/*
   $Header: /cvs/src/tdl/io.c,v 1.2 2001/08/20 22:38:00 richard Exp $
  
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

#include <string.h>
#include "tdl.h"

#define MAGIC1 0x99bb0001UL

static unsigned int count_length(struct links *x)/*{{{*/
{
  int n = 0;
  struct node *y;
  for (y = x->next; &y->chain != x; y = y->chain.next) n++;
  return n;
}
/*}}}*/
static unsigned int count_kids(struct node *x)/*{{{*/
{
  return count_length(&x->kids);
}
/*}}}*/
static void write_int(unsigned int n, FILE *out)/*{{{*/
{
  unsigned int a, b, c, d;
  a = (n >> 24) & 255;
  b = (n >> 16) & 255;
  c = (n >>  8) & 255;
  d = (n      ) & 255;
  fputc(a, out);
  fputc(b, out);
  fputc(c, out);
  fputc(d, out);
}
/*}}}*/
/*{{{  static unsigned int read_int(FILE *in)*/
static unsigned int read_int(FILE *in)
{
  
  unsigned int a, b, c, d;
  a = fgetc(in);
  b = fgetc(in);
  c = fgetc(in);
  d = fgetc(in);
  return ((a & 0xff) << 24) + ((b & 0xff) << 16) +
         ((c & 0xff) <<  8) +  (d & 0xff);
}

/*}}}*/
static void write_node(struct node *x, FILE *out)/*{{{*/
{
  int len, n_kids;
  struct node *kid;

  len = strlen(x->text);
  n_kids = count_kids(x);
  
  write_int(x->arrived, out);
  write_int(x->required_by, out);
  write_int(x->done, out);
  fputc(x->priority, out);
  write_int(len, out);
  fwrite(x->text, 1, len, out);

  write_int(n_kids, out);
  for (kid = x->kids.next; kid != (struct node *) &x->kids; kid = kid->chain.next)
  {
    write_node(kid, out);
  }
}
/*}}}*/
void write_database(FILE *out)/*{{{*/
{
  int n_kids;
  struct node *kid;

  n_kids = count_length(&top);
  
  write_int(MAGIC1, out);
  write_int(n_kids, out);
  for (kid = top.next; kid != (struct node *) &top; kid = kid->chain.next)
  {
    write_node(kid, out);
  }
}
/*}}}*/
/*{{{  static struct node *read_node(FILE *in)*/
static struct node *read_node(FILE *in)
{
  struct node *r = new_node();
  int len, n_kids, i;
  
  r->arrived     = read_int(in);
  r->required_by = read_int(in);
  r->done        = read_int(in);
  r->priority    = fgetc(in);
  if ((r->priority < PRI_UNKNOWN) || (r->priority > PRI_URGENT)) {
    r->priority = PRI_NORMAL;
  }
  len            = read_int(in);
  r->text        = new_array(char, len+1);
  fread(r->text, 1, len, in);
  r->text[len] = 0;
  
  n_kids         = read_int(in);

  for (i=0; i<n_kids; i++) {
    struct node *nn;
    nn = read_node(in);
    append_child(nn, r);
  }

  return r;
}

/*}}}*/
/*{{{  void read_database(FILE *in)*/
void read_database(FILE *in)
{
  int n_kids, i;
  unsigned int magic;

  /* FIXME : Ought to clear any existing database.  For initial version this
   * won't happen (only one load per invocation), and even later, we might just
   * take the memory leak. But for safety, ... */

  top.prev = (struct node *) &top;
  top.next = (struct node *) &top;
  

  if (feof(in)) {
    fprintf(stderr, "Can't read anything from database\n");
    exit(1);
  }

  magic = read_int(in);
  if (magic != MAGIC1) {
    fprintf(stderr, "Cannot parse database, wrong magic number\n");
    exit(1);
  }
  n_kids = read_int(in);
  for (i=0; i<n_kids; i++) {
    struct node *nn;
    nn = read_node(in);
    append_child(nn, NULL);
  }
}

/*}}}*/
