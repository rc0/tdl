/*
   $Header: /cvs/src/tdl/list.c,v 1.14 2002/07/18 22:13:58 richard Exp $
  
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

#include <time.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "tdl.h"

struct list_options {
  unsigned monochrome:1;
  unsigned show_all:1;
  unsigned verbose:1;
  unsigned set_depth:1;
  int depth;
};

#define INDENT_TAB 3

/*{{{ Colour definitions */
#define RED     "[31m[1m"
#define GREEN   "[32m"
#define YELLOW  "[33m[1m"
#define BLUE    "[34m"
#define MAGENTA "[35m"
#define CYAN    "[36m"
#define NORMAL  "[0m"
#define DIM     "[37m[2m"
#define DIMCYAN "[36m[2m"

/* Table to map priority levels to colours */
static char *colour_table[] = {
  NORMAL, BLUE, CYAN, NORMAL, YELLOW, RED
};

static char *priority_text[] = {
  "UNKNOWN!", "verylow", "low", "normal", "high", "urgent"
};

/*}}}*/
void do_indent(int indent)/*{{{*/
{
  int i;
  for (i=0; i<indent; i++) putchar(' ');
}
/*}}}*/
void do_bullet_indent(int indent)/*{{{*/
{
  int i;
  int n;
  n = indent - 2;
  for (i=0; i<indent; i++) putchar((i == n) ? '-' : ' ');
}
/*}}}*/
static void print_timestamp(int timestamp, char *leader, int indent, int monochrome)/*{{{*/
{
  char buffer[32];
  time_t now;
  long diff, days_ago, days_ahead;
  
  now = time(NULL);
  diff = now - timestamp;
  days_ago = (diff + ((diff > 0) ? 43200 : -43200)) / 86400;
  strftime(buffer, sizeof(buffer), "%a %d %b %Y %H:%M", 
           localtime((time_t *)&timestamp));
  do_indent(indent+2);
  if (days_ago < 0) {
    days_ahead = - days_ago;
    if (monochrome) {
      printf("%s: %s (%ld day%s ahead)\n", leader, buffer, days_ago, (days_ahead == 1) ? "" : "s");
    } else {
      printf("%s%s:%s %s %s(%ld day%s ahead)%s\n", GREEN, leader, NORMAL, buffer, MAGENTA, days_ahead, (days_ahead == 1) ? "" : "s", NORMAL);
    }
  } else {
    if (monochrome) {
      printf("%s: %s (%ld day%s ago)\n", leader, buffer, days_ago, (days_ago == 1) ? "" : "s");
    } else {
      printf("%s%s:%s %s (%ld day%s ago)\n", GREEN, leader, NORMAL, buffer, days_ago, (days_ago == 1) ? "" : "s");
    }
  }
}
/*}}}*/
static void count_kids(struct links *x, int *n_kids, int *n_done_kids, int *n_open_kids)/*{{{*/
{
  int nk, ndk;
  struct node *y;

  nk = ndk = 0;
  for (y = x->next;
       y != (struct node *) x;
       y = y->chain.next) {
    if (y->done) ndk++;
    nk++;
  }

  if (n_kids) *n_kids = nk;
  if (n_done_kids) *n_done_kids = ndk;
  if (n_open_kids) *n_open_kids = (nk - ndk);
  return;
}
/*}}}*/
static void print_details(struct node *y, int indent, int summarise_kids, const struct list_options *options, char *index_buffer, time_t now)/*{{{*/
{
  int is_done;
  int is_ignored;
  char *p;
  int n_kids, n_open_kids;

  is_done = (y->done > 0);
  is_ignored = (y->done == IGNORED_TIME);
  if (!options->show_all && is_done) return;

  do_indent(indent);
  count_kids(&y->kids, &n_kids, NULL, &n_open_kids);
  if (summarise_kids && (n_kids > 0)) {
    if (options->monochrome) {
      printf("%s [%d/%d] %s", index_buffer, n_open_kids, n_kids,
             (options->show_all && !options->verbose && is_ignored) ? "(IGNORED) : " : 
             (options->show_all && !options->verbose && is_done) ? "(DONE) : " : 
             (options->show_all && !options->verbose && (y->arrived > now)) ? "(DEFERRED) : " : ": ");
    } else {
      printf("%s%s %s[%d/%d]%s %s%s", GREEN, index_buffer, CYAN, n_open_kids, n_kids, NORMAL,
             (options->show_all && !options->verbose && is_ignored) ? BLUE "(IGNORED) " NORMAL :
             (options->show_all && !options->verbose && is_done) ? CYAN "(DONE) " NORMAL :
             (options->show_all && !options->verbose && (y->arrived > now)) ? MAGENTA "(DEFERRED) " : "",
             is_done ? DIMCYAN : colour_table[y->priority]);
    }
  } else {
    if (options->monochrome) {
      printf("%s %s", index_buffer,
             (options->show_all && !options->verbose && is_ignored) ? "(IGNORED) : " : 
             (options->show_all && !options->verbose && is_done) ? "(DONE) : " : 
             (options->show_all && !options->verbose && (y->arrived > now)) ? "(DEFERRED) : " : ": ");
    } else {
      printf("%s%s%s %s%s", GREEN, index_buffer, NORMAL,
             (options->show_all && !options->verbose && is_ignored) ? BLUE "(IGNORED) " NORMAL :
             (options->show_all && !options->verbose && is_done) ? CYAN "(DONE) " NORMAL :
             (options->show_all && !options->verbose && (y->arrived > now)) ? MAGENTA "(DEFERRED) " : "",
             is_done ? DIMCYAN : colour_table[y->priority]);
    }
  }
  for (p = y->text; *p; p++) {
    putchar(*p);
    if (*p == '\n') {
      do_indent(indent + 5);
    }
  }
  if (!options->monochrome) printf("%s", NORMAL);
  printf("\n");
  if (options->verbose) {
    print_timestamp(y->arrived, "Arrived", indent, options->monochrome);
    do_indent(indent + 2);
    if (options->monochrome) {
      printf("Priority: %s\n", priority_text[y->priority]);
    } else {
      printf("%sPriority: %s%s%s\n",
          GREEN, colour_table[y->priority], priority_text[y->priority], NORMAL);
    }
    if (y->required_by > 0) print_timestamp(y->required_by, "Required by", indent, options->monochrome);
    if (y->done > 0) print_timestamp(y->done, "Completed", indent, options->monochrome);
    printf("\n");
  }

}
/*}}}*/
static void list_chain(struct links *x, int indent, int depth, const struct list_options *options, char *index_buffer, enum Priority prio, time_t now, unsigned char *hits)/*{{{*/
{
  struct node *y;
  int idx, is_done;
  char component_buffer[8];
  char new_index_buffer[64];
  
  for (y = x->next, idx = 1;
       y != (struct node *) x;
       y = y->chain.next, ++idx) {
    
    is_done = (y->done > 0);
    /* Normally, don't show entries that have been done, or whose start time
     * has been deferred */
    if (!options->show_all && (is_done || (y->arrived > now))) continue;

    sprintf(component_buffer, "%d", idx);
    strcpy(new_index_buffer, index_buffer);
    if (strlen(new_index_buffer) > 0) {
      strcat(new_index_buffer, ".");
    }
    strcat(new_index_buffer, component_buffer);

    if (y->priority >= prio) {
      int summarise_kids = (options->set_depth && (options->depth == depth));
      if (hits[y->iscratch]) {
        print_details(y, indent, summarise_kids, options, new_index_buffer, now);
      }
    }

    /* Maybe list children regardless of priority assigned to parent. */
    if (!options->set_depth || (depth < options->depth)) {
      list_chain(&y->kids, indent + INDENT_TAB, depth + 1, options, new_index_buffer, prio, now, hits);
    }

  }
  return;
}
/*}}}*/
static void allocate_indices(struct links *x, int *idx)/*{{{*/
{
  struct node *y;
  for (y = x->next;
       y != (struct node *) x;
       y = y->chain.next) {

    y->iscratch = *idx;
    ++*idx;
    allocate_indices(&y->kids, idx);
  }
}
/*}}}*/
static void search_node(struct links *x, int max_errors, unsigned long *vecs, unsigned long hitvec, unsigned char *hits)/*{{{*/
{
  struct node *y;
  char *p;
  char *token;
  int got_hit;
  unsigned long r0, r1, r2, r3, nr0, nr1, nr2;

  for (y = x->next;
       y != (struct node *) x;
       y = y->chain.next) {

    token = y->text;

    switch (max_errors) {
      /* optimise common cases for few errors to allow optimizer to keep bitmaps
       * in registers */
      case 0:/*{{{*/
        r0 = ~0;
        got_hit = 0;
        for(p=token; *p; p++) {
          int idx = (unsigned int) *(unsigned char *)p;
          r0 = (r0<<1) | vecs[idx];
          if (~(r0 | hitvec)) {
            got_hit = 1;
            break;
          }
        }
        break;
        /*}}}*/
      case 1:/*{{{*/
        r0 = ~0;
        r1 = r0<<1;
        got_hit = 0;
        for(p=token; *p; p++) {
          int idx = (unsigned int) *(unsigned char *)p;
          nr0 = (r0<<1) | vecs[idx];
          r1  = ((r1<<1) | vecs[idx]) & ((r0 & nr0) << 1) & r0;
          r0  = nr0;
          if (~((r0 & r1) | hitvec)) {
            got_hit = 1;
            break;
          }
        }
        break;
  /*}}}*/
      case 2:/*{{{*/
        r0 = ~0;
        r1 = r0<<1;
        r2 = r1<<1;
        got_hit = 0;
        for(p=token; *p; p++) {
          int idx = (unsigned int) *(unsigned char *)p;
          nr0 =  (r0<<1) | vecs[idx];
          nr1 = ((r1<<1) | vecs[idx]) & ((r0 & nr0) << 1) & r0;
          r2  = ((r2<<1) | vecs[idx]) & ((r1 & nr1) << 1) & r1;
          r0  = nr0;
          r1  = nr1;
          if (~((r0 & r1& r2) | hitvec)) {
            got_hit = 1;
            break;
          }
        }
        break;
  /*}}}*/
      case 3:/*{{{*/
        r0 = ~0;
        r1 = r0<<1;
        r2 = r1<<1;
        r3 = r2<<1;
        got_hit = 0;
        for(p=token; *p; p++) {
          int idx = (unsigned int) *(unsigned char *)p;
          nr0 =  (r0<<1) | vecs[idx];
          nr1 = ((r1<<1) | vecs[idx]) & ((r0 & nr0) << 1) & r0;
          nr2 = ((r2<<1) | vecs[idx]) & ((r1 & nr1) << 1) & r1;
          r3  = ((r3<<1) | vecs[idx]) & ((r2 & nr2) << 1) & r2;
          r0  = nr0;
          r1  = nr1;
          r2  = nr2;
          if (~((r0 & r1 & r2 & r3) | hitvec)) {
            got_hit = 1;
            break;
          }
        }
        break;
        /*}}}*/
      default:
        assert(0); /* not allowed */
        break;
    }
    if (got_hit) {
      hits[y->iscratch] = 1;
    }
    search_node(&y->kids, max_errors, vecs, hitvec, hits);
  }
}
/*}}}*/
static void merge_search_condition(unsigned char *hits, int n_nodes, char *cond)/*{{{*/
{
  /* See "Fast text searching with errors, Sun Wu and Udi Manber, TR 91-11,
     University of Arizona.  I have been informed that this algorithm is NOT
     patented.  This implementation of it is entirely the work of Richard P.
     Curnow - I haven't looked at any related source (webglimpse, agrep etc) in
     writing this.
  */

  int max_errors;
  char *slash;
  char *substring;
  unsigned long a[256];
  unsigned long hit;
  int len, i;
  char *p;
  unsigned char *hit0;

  slash = strchr(cond, '/');
  if (!slash) {
    max_errors = 0;
    substring = cond;
  } else {
    substring = new_string(cond);
    substring[slash-cond] = '\0';
    max_errors = atoi(slash+1);
    if (max_errors > 3) {
      fprintf(stderr, "Can only match with up to 3 errors, ignoring patterh <%s>\n", cond);
      goto get_out;
    }
  }

  len = strlen(substring);
  if (len < 1 || len > 31) {
    fprintf(stderr, "Pattern must be between 1 and 31 characters\n");
    goto get_out;
  }
  
  /* Set array 'a' to all -1 values */
  memset(a, 0xff, 256 * sizeof(unsigned long));
  for (p=substring, i=0; *p; p++, i++) {
    unsigned char pc;
    pc = *(unsigned char *) p;
    a[(unsigned int) pc] &= ~(1UL << i);
    /* Make search case insensitive */
    if (isupper(pc)) {
      a[tolower((unsigned int) pc)] &= ~(1UL << i);
    }
    if (islower(pc)) {
      a[toupper((unsigned int) pc)] &= ~(1UL << i);
    }
  }
  hit = ~(1UL << (len-1));

  hit0 = new_array(unsigned char, n_nodes);
  memset(hit0, 0, n_nodes);
  
  /* Now scan each node against this match criterion */
  search_node(&top, max_errors, a, hit, hit0);
  for (i=0; i<n_nodes; i++) {
    hits[i] &= hit0[i];
  }
  free(hit0);

get_out:
  if (substring != cond) {
    free(substring);
  }
  return;
}
/*}}}*/
int process_list(char **x)/*{{{*/
{
  struct list_options options;
  int options_done = 0;
  int any_paths = 0;
  char index_buffer[64];
  char *y;
  enum Priority prio = PRI_NORMAL, prio_to_use, node_prio;
  int prio_set = 0;
  time_t now = time(NULL);
  
  unsigned char *hits;
  int node_index, n_nodes;

  if (getenv("TDL_LIST_MONOCHROME") != 0) {
    options.monochrome = 1;
  }

  options.monochrome = 0;
  options.show_all = 0;
  options.verbose = 0;
  options.set_depth = 0;
  
  /* Initialisation to support searching */
  node_index = 0;
  allocate_indices(&top, &node_index);
  n_nodes = node_index;

  hits = n_nodes ? new_array(unsigned char, n_nodes) : NULL;

  /* all nodes match until proven otherwise */
  memset(hits, 1, n_nodes);
  
  while ((y = *x) != 0) {
    /* An argument starting '1' or '+1' or '+-1' (or '-1' after '--') is
     * treated as the path of the top node to show */
    if (isdigit(y[0]) ||
        (options_done && (y[0] == '-') && isdigit(y[1])) ||
        ((y[0] == '+') &&
         (isdigit(y[1]) ||
          ((y[1] == '-' && isdigit(y[2])))))) {
      
      struct node *n = lookup_node(y, 0, NULL);
      int summarise_kids;

      if (!n) return -1;
      
      any_paths = 1;
      index_buffer[0] = '\0';
      strcpy(index_buffer, y);
      summarise_kids = (options.set_depth && (options.depth==0));
      if (hits[n->iscratch]) {
        print_details(n, 0, summarise_kids, &options, index_buffer, now);
      }
      if (!options.set_depth || (options.depth > 0)) {
        node_prio = n->priority;

        /* If the priority has been set on the cmd line, always use that.
         * Otherwise, use the priority from the specified node, _except_ when
         * that is higher than normal, in which case use normal. */
        prio_to_use = (prio_set) ? prio : ((node_prio > prio) ? prio : node_prio);
        list_chain(&n->kids, INDENT_TAB, 0, &options, index_buffer, prio_to_use, now, hits);
      }
    } else if ((y[0] == '-') && (y[1] == '-')) {
      options_done = 1;
    } else if (y[0] == '-') {
      while (*++y) {
        switch (*y) {
          case 'v':
            options.verbose = 1;
            break;
          case 'a':
            options.show_all = 1;
            break;
          case 'm':
            options.monochrome = 1;
            break;
          case '1': case '2': case '3':
          case '4': case '5': case '6': 
          case '7': case '8': case '9':
            options.set_depth = 1;
            options.depth = (*y) - '1';
            break;
          default:
            fprintf(stderr, "Unrecognized option : -%c\n", *y);
            break;
        }
      }
    } else if (y[0] == '/') {
      /* search expression */
      merge_search_condition(hits, n_nodes, y+1);
       
    } else {
      int error;
      prio = parse_priority(y, &error);
      if (error < 0) return error;
      prio_set = 1;
    }

    x++;
  }
  
  if (!any_paths) {
    index_buffer[0] = 0;
    list_chain(&top, 0, 0, &options, index_buffer, prio, now, hits);
  }

  if (hits) free(hits);
  
  return 0;
}
/*}}}*/
