/*
   $Header: /cvs/src/tdl/io.c,v 1.6 2003/08/06 23:23:00 richard Exp $
  
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
#include <assert.h>
#include "tdl.h"

#define MAGIC1 0x99bb0001L
#define MAGIC2 0x99bb0002L

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
static void write_node(struct node *x, FILE *out, int version)/*{{{*/
{
  int len, n_kids;
  struct node *kid;

  len = strlen(x->text);
  n_kids = count_kids(x);
  
  if (version >= 2) {
    write_int(x->entered, out);
  }
  write_int(x->arrived, out);
  write_int(x->required_by, out);
  write_int(x->done, out);
  fputc(x->priority, out);
  if (version >= 2) {
    int name_len;
    if (x->name) {
      name_len = strlen(x->name);
      write_int(name_len, out);
      fwrite(x->name, 1, name_len, out);
    } else {
      write_int(0, out); /* empty name */
    }
    write_int(0, out); /* placeholder for number of notes attached to node */
  }
  write_int(len, out);
  fwrite(x->text, 1, len, out);

  write_int(n_kids, out);
  for (kid = x->kids.next; kid != (struct node *) &x->kids; kid = kid->chain.next)
  {
    write_node(kid, out, version);
  }
}
/*}}}*/
void write_database(FILE *out, struct links *from, int version)/*{{{*/
{
  int n_kids;
  struct node *kid;

  n_kids = count_length(from);
  
  switch (version) {
    case 1:
      write_int(MAGIC1, out);
      break;
    case 2:
      write_int(MAGIC2, out);
      break;
    default:
      assert(0);
      break;
  }
  write_int(n_kids, out);
  for (kid = from->next; kid != (struct node *) from; kid = kid->chain.next)
  {
    write_node(kid, out, version);
  }
}
/*}}}*/
/*{{{  static struct node *read_node(FILE *in)*/
static struct node *read_node(FILE *in, int version)
{
  struct node *r = new_node();
  int len, n_kids, i;
  
  if (version >= 2) {
    r->entered   = read_int(in);
  }
  r->arrived     = read_int(in);
  if (version < 2) {
    r->entered   = r->arrived;
  }
  r->required_by = read_int(in);
  r->done        = read_int(in);
  r->priority    = fgetc(in);
  if ((r->priority < PRI_UNKNOWN) || (r->priority > PRI_URGENT)) {
    r->priority = PRI_NORMAL;
  }
  if (version >= 2) {
    int name_len, n_notes;
    name_len     = read_int(in);
    if (name_len > 0) {
      r->name    = new_array(char, name_len+1);
      fread(r->name, 1, name_len, in);
    } else {
      r->name    = NULL;
    }
    n_notes      = read_int(in);
    r->notes.next = r->notes.prev = &r->notes;
    /* TODO : read this stuff in properly. */
  } else {
    r->name      = NULL;
    r->notes.next = r->notes.prev = &r->notes;
    /* TODO : Initialise the structure fields if a v1 db is being read */
  }
  len            = read_int(in);
  r->text        = new_array(char, len+1);
  fread(r->text, 1, len, in);
  r->text[len] = 0;
  
  n_kids         = read_int(in);

  for (i=0; i<n_kids; i++) {
    struct node *nn;
    nn = read_node(in, version);
    prepend_child(nn, r);
  }

  return r;
}

/*}}}*/
/*{{{  void read_database(FILE *in)*/
int read_database(FILE *in, struct links *to, int *read_version)
{
  int n_kids, i;
  unsigned int magic;
  int version;

  /* FIXME : Ought to clear any existing database.  For initial version this
   * won't happen (only one load per invocation), and even later, we might just
   * take the memory leak. But for safety, ... */

  to->prev = (struct node *) to;
  to->next = (struct node *) to;
  
  if (feof(in)) {
    fprintf(stderr, "Can't read anything from database\n");
    return -1;
  }

  magic = read_int(in);
  if (magic == MAGIC1) {
    version = 1;
  } else if (magic == MAGIC2) {
    version = 2;
  } else {
    fprintf(stderr, "Cannot parse database, wrong magic number\n");
    return -1;
  }
  n_kids = read_int(in);
  for (i=0; i<n_kids; i++) {
    struct node *nn;
    nn = read_node(in, version);
    prepend_node(nn, to);
  }

  *read_version = version;
  return 0;
}

/*}}}*/
