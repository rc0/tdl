/*
   $Header: /cvs/src/tdl/util.c,v 1.9 2003/07/17 22:35:04 richard Exp $
  
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
  struct links *x;
  struct node *y = NULL;
  struct node *narrow_top;

  narrow_top = get_narrow_top();
  if (narrow_top) {
    /* Special case to allow user to do operations on the node to which the
     * view is currently narrowed. (This doesn't apply to 'top' which is just a
     * skeleton entry.) */
    if (!strcmp(path, ".")) {
      return narrow_top;
    }
    x = &(narrow_top->kids);
  } else {
    x = &top;
  }
  
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

static void add_noderef_to_list(struct nodelist *list, struct node *n, int flag)/*{{{*/
{
  struct nodelist *new_node = new(struct nodelist);
  new_node->node = n;
  new_node->flag = flag;
  new_node->next = list;
  new_node->prev = list->prev;
  list->prev->next = new_node;
  list->prev = new_node;
  return;
}
/*}}}*/
enum what_lookup {
  WL_BOTH,
  WL_START,
  WL_END
};

static struct node *lookup_one_node(struct links *x, const char *s, const char *e, enum what_lookup wl)/*{{{*/
{
  char *temp;
  int len = e - s;
  int index;
  struct node *result;
  
  temp = new_array(char, len + 1);
  strncpy(temp, s, len);
  temp[len] = 0;
  /* Eventually this could be more sophisticated */
  if (sscanf(temp, "%d", &index) != 1) {
    fprintf(stderr, "Could not extract index from <%s>\n", temp);
    result = NULL;
  } else {
    int aindex, direction, tindex;
    struct node *y;

    if (x->next == (struct node *)x) {
      fprintf(stderr, "Tree not that deep\n");
      result = NULL;
      goto out;
    }

    if (index > 0) {
      direction = 1;
      aindex = index;
    } else if (index < 0) {
      direction = 0;
      aindex = -index;
    } else {
      switch (wl) {
        case WL_BOTH:
          fprintf(stderr, "Zero index not allowed\n");
          result = NULL;
          goto out;
        case WL_START:
          result = x->next;
          goto out;
        case WL_END:
          result = x->prev;
          goto out;
      }
    }

    for (y = direction ? x->next : x->prev, tindex = aindex; --tindex; ) {
      y = direction ? y->chain.next : y->chain.prev;
      if (y == (struct node *) x) {
        fprintf(stderr, "Can't find entry %d for XXth component of path ...\n",
            index);
        result = NULL;
        goto out;
      }
    }

    result = y;
  }

out: 
  free(temp);
  return result;
}
/*}}}*/
static int node_after_node(struct node *n1, struct node *n2, struct links *links)/*{{{*/
{
  /* FIXME : provide a real definition! */
  struct node *n;
  for (n = n1; ; n = n->chain.next) {
    if (n == (struct node *) links) return 0;
    if (n == n2) break;
  }
  return 1;
}
/*}}}*/
static void lookup_range(struct links *x, const char *start, const char *end, struct nodelist *list, int flag)/*{{{*/
{
  char *dash;
  struct node *n1, *n2, *n;
  
  dash = strchr(((start[0]=='-') ? (start+1) : start), '-');
  if (!dash || (dash >= end)) {
    /* single term */
    n1 = lookup_one_node(x, start, end, WL_BOTH);
    if (!n1) return; /* Don't add anything if faulty */
    add_noderef_to_list(list, n1, flag);
  } else {
    /* two terms */
    n1 = lookup_one_node(x, start, dash, WL_START);
    n2 = lookup_one_node(x, dash+1, end, WL_END);
    if (!n1 || !n2) return; /* Don't add anything if faulty */
    if (!node_after_node(n1, n2, x)) {
      fprintf(stderr, "Start node after end node\n"); 
      return; /* Return if out of order. */
    }
    /* Add whole range (inclusive) to list. */
    for (n=n1; ; n = n->chain.next) {
      add_noderef_to_list(list, n, flag);
      if (n == n2) break;
    }
  }
}
/*}}}*/
static void lookup_final(struct links *x, const char *p, struct nodelist *list, int flag)/*{{{*/
{
  const char *q;
  do {
    q = strchr(p, ',');
    if (!q) {
      /* Last range in the string */
      q = p;
      while (*q) q++;
      lookup_range(x, p, q, list, flag);
      break;
    } else {
      lookup_range(x, p, q, list, flag);
      p = q + 1; /* Character after the comma */
    }
  } while (1);
  return;
}
/*}}}*/
static struct links *lookup_non_final(struct links *x, const char *p, const char *dotpos)/*{{{*/
{
  /* Check that [p,dotpos) is a valid index, then find that index in x.
     Actually, we're less picky, only require that we can read an integer
     starting at 'p'. */
  int idx, aidx, tidx, direction;
  struct node *y;
  
  if (sscanf(p, "%d", &idx) != 1) {
    fprintf(stderr, "Bad path expression found, starting [%s]\n", p);
    return NULL;
  }
  if (idx > 0) {
    direction = 1;
    aidx = idx;
  } else if (idx < 0) {
    direction = 0;
    aidx = -idx;
  } else {
    fprintf(stderr, "Zero index not allowed\n");
    return NULL;
  }

  if (x->next == (struct node *)x) {
    fprintf(stderr, "Tree not that deep\n");
    return NULL;
  }

  for (y = direction ? x->next : x->prev, tidx = aidx; --tidx; ) {
    y = direction ? y->chain.next : y->chain.prev;
    if (y == (struct node *) x) {
      fprintf(stderr, "Can't find entry %d for XXth component of path ...\n",
          idx);
      return NULL;
    }
  }
  return &(y->kids);

}
/*}}}*/
void lookup_nodes(const char *path, struct nodelist *list, int flag)/*{{{*/
{
  /* Find nodes matched by path, and append them to the list */
  struct node *narrow_top;
  struct links *our_top, *x;
  const char *p;

  narrow_top = get_narrow_top();
  if (narrow_top) {
    if (!strcmp(path, ".")) {
      add_noderef_to_list(list, narrow_top, flag);
    } else {
      our_top = &narrow_top->kids;
    }
  } else {
    our_top = &top;
  }

  /* General syntax:
   * a.b.c.X where a, b, c are integers (eventually may be names when we add node-naming)
   * X is a list of comma-separated ranges, each range is a single integer/name, or a
   * range of 2 integer/names separated by hyphen. */
  
  x = our_top;
  p = path;
  do {
    const char *dotpos;
    dotpos = strchr(p, '.');
    if (!dotpos) {
      /* Final component */
      lookup_final(x, p, list, flag);
      return;
    } else {
      /* Non-final component */
      x = lookup_non_final(x, p, dotpos);
      /* Failure - just don't add anything to the nodelist. */
      if (!x) return;
    }
    p = dotpos + 1;
  } while (1);

}
/*}}}*/
struct nodelist *make_nodelist(void)/*{{{*/
{
  struct nodelist *result;
  result = new(struct nodelist);
  result->next = result->prev = result;
  return result;
}
/*}}}*/
void free_nodelist(struct nodelist *list)/*{{{*/
{
  struct nodelist *next, *x;
  for (x = list; x != list; x = x->next) {
    next = x->next;
    free(x);
  }
  return;
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
    struct node *narrow_top;
    narrow_top = get_narrow_top();
    prepend_node(child, narrow_top ? &narrow_top->kids : &top);
  }
}
/*}}}*/
