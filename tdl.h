/*
   $Header: /cvs/src/tdl/tdl.h,v 1.15 2002/05/21 22:47:26 richard Exp $
  
   tdl - A console program for managing to-do lists
   Copyright (C) 2001,2002  Richard P. Curnow

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

typedef char** (*Completer)(char *, int);

/* Command table (shared between dispatcher and readline completer) */
struct command {/*{{{*/
  char *name; /* add, remove etc */
  char *shortcut; /* tdla etc */
  int (*func)(char **); /* ptr to function that actually does the work for this cmd */
  char *descrip; /* One line description */
  char *synopsis; /* Description of parameters */
  char ** (*completer)(char *, int); /* Function to generate completions */
  unsigned char  dirty; /* 1 if operation can dirty the database, 0 if read-only */
  unsigned char  load_db; /* 1 if cmd requires current database to be loaded first */
  unsigned char  matchlen; /* number of characters to make command unambiguous */
  unsigned char  interactive_ok; /* 1 if OK to use interactively. */
  unsigned char  non_interactive_ok; /* 1 if OK to use from command line */
};
/*}}}*/

extern struct command cmds[];
extern int n_cmds;

/* Function prototypes. */

/* In io.c */
int read_database(FILE *in, struct links *to);
void write_database(FILE *out, struct links *from);
struct node *new_node(void);
void append_node(struct node *n, struct links *l);
void append_child(struct node *child, struct node *parent);
void clear_flags(struct links *x);
int has_kids(struct node *x);

/* In list.c */
void do_indent(int indent);
void do_bullet_indent(int indent);
int process_list(char **x);

/* In report.c */
int process_report(char **x);
long read_interval(char *xx);

/* In util.c */
int count_args(char **x);
int include_descendents(char *x);
struct node *lookup_node(char *path, int allow_zero_index, struct node **parent);
enum Priority parse_priority(char *priority, int *error);
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
int process_done(char **x);
int process_undo(char **x);

/* In add.c */
int process_add(char **x);
int process_log(char **x);
int process_edit(char **x);

/* In remove.c */
int process_remove(char **x);

/* In purge.c */
int process_purge(char **x);

/* In move.c */
int process_above(char **x);
int process_below(char **x);
int process_into(char **x);

/* In impexp.c */
int process_export(char **x);
int process_import(char **x);

/* In dates.c */
time_t parse_date(char *d, time_t ref, int default_positive, int *error);

/* In main.c */
void dispatch(char **argv);
void load_database_if_not_loaded(void);

/* In inter.c */
void interactive(void);
char *interactive_text(char *prompt, char *initval, int *is_blank, int *error);
char **complete_help(char *, int);
char **complete_list(char *, int);
char **complete_priority(char *, int);
char **complete_done(char *, int);

#endif /* TDL_H */
          
