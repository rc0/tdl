/*
   $Header: /cvs/src/tdl/tdl.h,v 1.7 2001/10/14 22:08:28 richard Exp $
  
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
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
  char *scratch; /* For functions to attach stuff to nodes */
  char flag;
};

extern struct links top;

/* Memory macros */

#define new_string(s) strcpy((char *) malloc(1+strlen(s)), (s))
#define extend_string(x,s) (strcat(realloc(x, (strlen(x)+strlen(s)+1)), s))
#define new(T) (T *) malloc(sizeof(T))
#define new_array(T, n) (T *) malloc(sizeof(T) * (n))
#define grow_array(T, n, oldX) (T *) ((oldX) ? realloc(oldX, (sizeof(T) * (n))) : malloc(sizeof(T) * (n)))

/* Function prototypes. */

/* In io.c */
void read_database(FILE *in, struct links *to);
void write_database(FILE *out, struct links *from);
struct node *new_node(void);
void append_node(struct node *n, struct links *l);
void append_child(struct node *child, struct node *parent);
void clear_flags(struct links *x);
int has_kids(struct node *x);

/* In list.c */
void do_indent(int indent);
void do_bullet_indent(int indent);
void process_list(char **x);

/* In report.c */
void process_report(char **x);
long read_interval(char *xx);

/* In util.c */
int count_args(char **x);
int include_descendents(char *x);
struct node *lookup_node(char *path, int allow_zero_index, struct node **parent);
enum Priority parse_priority(char *priority);
void clear_flags(struct links *x);
void mark_all_descendents(struct node *n);
int has_kids(struct node *x);
struct node *new_node(void);
void free_node(struct node *x);
void append_node(struct node *n, struct links *l);
void prepend_node(struct node *n, struct links *l);
void prepend_child(struct node *child, struct node *parent);

/* In done.c */
int has_open_child(struct node *y);
void process_done(char **x);

/* In add.c */
void process_add(char **x, int set_done);
void process_edit(char **x);

/* In remove.c */
void process_remove(char **x);

/* In purge.c */
void process_purge(char **x);

/* In move.c */
void process_move(char **x, int below_not_above, int into_parent);

/* In impexp.c */
void process_export(char **x);
void process_import(char **x);

/* In dates.c */
time_t parse_date(char *d, time_t ref, int default_positive);

#endif /* TDL_H */
          
