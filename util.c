/*
   $Header: /cvs/src/tdl/util.c,v 1.7 2002/07/22 20:38:11 richard Exp $
  
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

#include <ctype.h>
#include "tdl.h"

int count_args(char **x)/*{{{*/
{
  int n = 0;
  while (*x) {
    n++;
    x++;
  }
  return n;
}
/*}}}*/
int include_descendents(char *x)/*{{{*/
{
  /* Check if string ends in ... . If it does, truncate that off. */
  int len;
  int result = 0;
  len = strlen(x);
  if (len >= 4) {
    if (!strcmp(x + (len-3), "...")) {
      result = 1;
      x[len-3] = 0;
    }
  }
  return result;
}
/*}}}*/
struct node *lookup_node(char *path, int allow_zero_index, struct node **parent)/*{{{*/
{
  char *p = path;
  int n, nc, idx, tidx, aidx, ncomp, comp10;
  int direction;
  struct links *x = &top;
  struct node *y = NULL;

  ncomp = 1;
  if (parent) *parent = NULL;

  while (*p) {
    n = sscanf(p, "%d%n", &idx, &nc);
    if (n != 1) {
      fprintf(stderr, "Bad path expression found, starting [%s]\n", p);
      return NULL;
    }
    p += nc;
    
    if (idx > 0) {
      direction = 1;
      aidx = idx;
    } else if (idx < 0) {
      direction = 0;
      aidx = -idx;
    } else {
      if (allow_zero_index) {
        if (*p) {
          fprintf(stderr, "Zero index only allowed as last component\n");
          return NULL;
        } else {
          /* This is a special cheat to allow inserting entries at
             the start or end of a chain for the 'above' and
             'below' commands */
          return (struct node *) x;
        }
      } else {
        fprintf(stderr, "Zero in index not allowed\n");
        return NULL;
      }
    }
      
    if (x->next == (struct node *) x) {
      fprintf(stderr, "Path [%s] doesn't exist - tree not that deep\n", path);
      return NULL;
    }

    comp10 = ncomp % 10;

    for (y = direction ? x->next : x->prev, tidx = aidx; --tidx;) {
      
      y = direction ? y->chain.next : y->chain.prev;

      if (y == (struct node *) x) {
        fprintf(stderr, "Can't find entry %d for %d%s component of path %s\n",
                idx, ncomp,
                (comp10 == 1) ? "st" :
                (comp10 == 2) ? "nd" : 
                (comp10 == 3) ? "rd" : "th",
                path);
        return NULL;
      }
    }

    if (*p == '.') {
      p++;
      x = &y->kids;
      if (parent) *parent = y;
    }

    ncomp++;
  }

  return y;
}
/*}}}*/
enum Priority parse_priority(char *priority, int *error)/*{{{*/
{
  enum Priority result;
  int is_digit;
  
  if (!priority) {
  	*error = -1;
    return PRI_UNKNOWN;
  } else {
  
  	is_digit = isdigit(priority[0]);
  
  	if (is_digit) {
    	int value = atoi(priority);
    	result = (value >= PRI_URGENT) ? PRI_URGENT :
             	 (value <= PRI_VERYLOW) ? PRI_VERYLOW :
             	 (enum Priority) value;
  	} else {
      int len = strlen(priority);
    	if (!strncmp(priority, "urgent", len)) {
        result = PRI_URGENT;
    	} else if (!strncmp(priority, "high", len)) {
    		result = PRI_HIGH;
    	} else if (!strncmp(priority, "normal", len)) {
    	 	result = PRI_NORMAL;
    	} else if (!strncmp(priority, "low", len)) {
    		result = PRI_LOW;
    	} else if (!strncmp(priority, "verylow", len)) {
        result = PRI_VERYLOW;
    	} else {
        fprintf(stderr, "Can't parse priority '%s'\n", priority);
        *error = -1;
     	 	return PRI_UNKNOWN; /* bogus */
    	}
  	}
  }
  *error = 0;
  return result;
}
/*}}}*/
void clear_flags(struct links *x)/*{{{*/
{
  struct node *y;
  for (y = x->next; y != (struct node *) x; y = y->chain.next) {
    y->flag = 0;
    if (has_kids(y)) {
      clear_flags(&y->kids);
    }
  }
}
/*}}}*/
void mark_all_descendents(struct node *n)/*{{{*/
{
  struct node *y;
  for (y = n->kids.next; y != (struct node *) &n->kids; y = y->chain.next) {
    y->flag = 1;
    if (has_kids(y)) {
      mark_all_descendents(y);
    }
  }
}
/*}}}*/
int has_kids(struct node *x)/*{{{*/
{
  return (x->kids.next != (struct node *) &x->kids);
}
/*}}}*/
struct node *new_node(void)/*{{{*/
{
  struct node *result = new (struct node);
  result->parent = NULL;
  result->text = NULL;
  result->priority = PRI_NORMAL;
  result->arrived = result->required_by = result->done = 0U;
  result->kids.next = result->kids.prev = (struct node *) &result->kids;
  result->chain.next = result->chain.prev = (struct node *) &result->chain;
  return result;
}
/*}}}*/
void free_node(struct node *x)/*{{{*/
{
  /* FIXME : To be written */


}
/*}}}*/
void append_node(struct node *n, struct links *l)/*{{{*/
{
  n->chain.next = l->next;
  n->chain.prev = (struct node *) l;
  l->next->chain.prev = n;
  l->next = n;
}
/*}}}*/
void prepend_node(struct node *n, struct links *l)/*{{{*/
{
  n->chain.prev = l->prev;
  n->chain.next = (struct node *) l;
  l->prev->chain.next = n;
  l->prev = n;
}
/*}}}*/
void prepend_child(struct node *child, struct node *parent)/*{{{*/
{
  child->parent = parent;
  if (parent) {
    prepend_node(child, &parent->kids);
  } else {
    prepend_node(child, &top);
  }
}
/*}}}*/
