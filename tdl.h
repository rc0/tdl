/*
   $Header: /cvs/src/tdl/tdl.h,v 1.1 2001/08/19 22:09:24 richard Exp $
  
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

#ifndef TDL_H
#define TDL_H

#include <stdio.h>

enum Priority {
  PRI_UNKNOWN = 0,
  PRI_VERYLOW = 1,
  PRI_LOW     = 2,
  PRI_NORMAL  = 3,
  PRI_HIGH    = 4,
  PRI_URGENT  = 5
};

struct node;

struct links {
  struct node *next;
  struct node *prev;
};

struct node {
  struct links chain;
  struct links kids;
  char *text;
  struct node *parent;
  enum Priority priority;
  long arrived;
  long required_by;
  long done;
  char flag;
};

extern struct links top;

/* Function prototypes. */

/* In io.c */
void read_database(FILE *in);
void write_database(FILE *out);
struct node *new_node(void);
void append_node(struct node *n, struct links *l);
void append_child(struct node *child, struct node *parent);
void clear_flags(struct links *x);
int has_kids(struct node *x);

#endif /* TDL_H */
          
