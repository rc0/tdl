/*
   $Header: /cvs/src/tdl/main.c,v 1.1 2001/08/19 22:09:24 richard Exp $
  
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

#include "tdl.h"
#include "memory.h"
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* The name of the database file (in whichever directory it may be) */
#define DBNAME ".tdldb"

/* The database */
struct links top;

/* Set if db doesn't exist in this directory */
static int no_database_here = 0;

/*{{{ Colour definitions */
#define RED     "[31m[1m"
#define GREEN   "[32m"
#define YELLOW  "[33m[1m"
#define BLUE    "[34m"
#define MAGENTA "[35m"
#define CYAN    "[36m"
#define NORMAL  "[0m"
#define DIM     "[37m[2m"

/* Table to map priority levels to colours */
static char *colour_table[] = {
  NORMAL, BLUE, CYAN, NORMAL, YELLOW, RED
};

static char *priority_text[] = {
  "UNKNOWN!", "verylow", "low", "normal", "high", "urgent"
};

/*}}}*/

static int count_args(char **x)/*{{{*/
{
  int n = 0;
  while (*x) {
    n++;
    x++;
  }
  return n;
}
/*}}}*/
static struct node *lookup_node(char *path)/*{{{*/
{
  char *p = path;
  int n, nc, idx, tidx, ncomp, comp10;
  struct links *x = &top;
  struct node *y = NULL;

  ncomp = 1;

  while (*p) {
    if (x->next == (struct node *) x) {
      fprintf(stderr, "Path [%s] doesn't exist - tree not that deep\n", path);
      exit(1);
    }

    n = sscanf(p, "%d%n", &idx, &nc);
    if (n != 1) {
      fprintf(stderr, "Bad path expression found, starting [%s]\n", p);
      exit(1);
    }
    p += nc;

    comp10 = ncomp % 10;
    for (y = x->next, tidx = idx; --tidx;) {
      
      y = y->chain.next;

      if (y == (struct node *) x) {
        fprintf(stderr, "Can't find entry %d for %d%s component of path %s\n",
                idx, ncomp,
                (comp10 == 1) ? "st" :
                (comp10 == 2) ? "nd" : 
                (comp10 == 3) ? "rd" : "th",
                path);
        exit(1);
      }
    }

    if (*p == '.') {
      p++;
      x = &y->kids;
    }

    ncomp++;
  }

  return y;
}
/*}}}*/
static enum Priority parse_priority(char *priority)/*{{{*/
{
  enum Priority result;
  if (!strcmp(priority, "urgent") || (atoi(priority) >= PRI_URGENT)) {
    result = PRI_URGENT;
  } else if (!strcmp(priority, "high") || (atoi(priority) == PRI_HIGH)) {
    result = PRI_HIGH;
  } else if (!strcmp(priority, "normal") || (atoi(priority) == PRI_NORMAL)) {
    result = PRI_NORMAL;
  } else if (!strcmp(priority, "low") || (atoi(priority) == PRI_LOW)) {
    result = PRI_LOW;
  } else if (!strcmp(priority, "verylow") || (atoi(priority) <= PRI_VERYLOW)) {
    result = PRI_VERYLOW;
  } else {
    fprintf(stderr, "Can't parse new priority '%s'\n", priority);
    exit(1);
  }
  
  return result;
}
/*}}}*/
static void process_add(char **x, int set_done)/*{{{*/
{
  /* Expect 1 argument, the string to add.  Need other options at some point. */
  time_t now;
  struct node *nn;
  int argc = count_args(x);
  char *text = NULL;
  char *parent_path = NULL;
  struct node *parent = NULL;
  enum Priority priority = PRI_NORMAL;
  int set_priority = 0;
  char *x0;

  switch (argc) {
    case 1:
      text = x[0];
      break;
    case 2:
      text = x[1];
      x0 = x[0];
      if (isdigit(x0[0])) {
        parent_path = x[0];
      } else {
        priority = parse_priority(x0);
        set_priority = 1;
      }
      break;
    case 3:
      text = x[2];
      priority = parse_priority(x[1]);
      set_priority = 1;
      parent_path = x[0];
      break;

    default:
      fprintf(stderr, "add requires 1, 2 or 3 arguments\n");
      break;
  }

  if (parent_path) {
    parent = lookup_node(parent_path);
  } else {
    parent = NULL;
  }
  
  now = time(NULL);
  nn = new_node();
  nn->text = new_string(text);
  nn->arrived = (long) now;
  if (set_done) {
    nn->done = (long) now;
  }
  
  nn->priority = (parent && !set_priority) ? parent->priority
                                           : priority;

  append_child(nn, parent);

  /* Clear done status of parents - they can't be any longer! */
  while (parent) {
    parent->done = 0;
    parent = parent->parent;
  }
  
}
/*}}}*/
static void do_indent(int indent)/*{{{*/
{
  int i;
  for (i=0; i<indent; i++) putchar(' ');
}
/*}}}*/
static void print_timestamp(int timestamp, char *leader, int indent)/*{{{*/
{
  char buffer[32];
  time_t now;
  long diff, days_ago;
  
  now = time(NULL);
  diff = now - timestamp;
  days_ago = diff / 86400;
  strftime(buffer, sizeof(buffer), "%a %d %b %Y %H:%M", 
           localtime((time_t *)&timestamp));
  do_indent(indent+2);
  printf("%s%s:%s %s (%ld day%s ago)\n", GREEN, leader, NORMAL, buffer, days_ago, (days_ago == 1) ? "" : "s");
}
/*}}}*/
static void list_chain(struct links *x, int indent, int verbose, int all, char *index_buffer)/*{{{*/
{
  struct node *y;
  int idx, is_done;
  char component_buffer[8];
  char new_index_buffer[64];
  
  for (y = x->next, idx = 1;
       y != (struct node *) x;
       y = y->chain.next, ++idx) {
    
    is_done = (y->done > 0);
    if (!all && is_done) continue;

    do_indent(indent);
    sprintf(component_buffer, "%d", idx);
    strcpy(new_index_buffer, index_buffer);
    if (strlen(new_index_buffer) > 0) {
      strcat(new_index_buffer, ".");
    }
    strcat(new_index_buffer, component_buffer);
    printf("%s%s%s %s%s%s%s\n", GREEN, new_index_buffer, NORMAL,
           (all && !verbose && is_done) ? CYAN "(DONE) " NORMAL : "",
           is_done ? CYAN : colour_table[y->priority],
           y->text, NORMAL);
    if (verbose) {
      print_timestamp(y->arrived, "Arrived", indent);
      do_indent(indent + 2);
      printf("%sPriority: %s%s%s\n",
             GREEN, colour_table[y->priority], priority_text[y->priority], NORMAL);
      if (y->required_by > 0) print_timestamp(y->required_by, "Required by", indent);
      if (y->done > 0) print_timestamp(y->done, "Completed", indent);
      printf("\n");
    }

    list_chain(&y->kids, indent + 3, verbose, all, new_index_buffer);
  }
  return;
}
/*}}}*/
static void process_list(char **x)/*{{{*/
{
  int verbose = 0;
  int all = 0; /* Even show done nodes */
  char index_buffer[2];

  while (*x) {
    if (!strcmp(*x, "-v")) verbose = 1;
    if (!strcmp(*x, "-a")) all = 1;
    x++;
  }
  
  index_buffer[0] = 0;

  list_chain(&top, 0, verbose, all, index_buffer);
}
/*}}}*/
static void process_remove(char **x)/*{{{*/
{
  struct node *n;

  while (*x) {
    n = lookup_node(*x);
    if (has_kids(n)) {
      fprintf(stderr, "Cannot delete path %s, it still has child entries\n", *x);
    } else {
      /* Just unlink from the chain for now, don't bother about full clean up. */
      struct node *next, *prev;
      next = n->chain.next;
      prev = n->chain.prev;
      prev->chain.next = next;
      next->chain.prev = prev;
    }
    x++;
  }
}
/*}}}*/
static void process_done(char **x)/*{{{*/
{
  struct node *n;
  struct node *y;
  int has_open_child;

  while (*x) {
    n = lookup_node(*x);
    has_open_child = 0;
    
    if (has_kids(n)) {
      for (y = n->kids.next; y != (struct node *) &n->kids; y = y->chain.next) {
        if (y->done == 0) {
          has_open_child = 1;
          break;
        }
      }
      if (has_open_child) {
        fprintf(stderr, "Cannot mark path %s done, it has child entries that are not complete\n", *x);
      }
    }
    
    if (!has_open_child) {
      time_t now;
      now = time(NULL);
      n->done = now;
    }
    x++;
  }
}
/*}}}*/
static void process_edit(char **x) {/*{{{*/
  int argc;
  struct node *n;

  argc = count_args(x);
  if (argc != 2) {
    fprintf(stderr, "Usage: edit <path> <new_text>\n");
    exit(1);
  }

  n = lookup_node(x[0]);
  free(n->text);
  n->text = new_string(x[1]);
  return;
}
/*}}}*/
static void process_create(char **x)/*{{{*/
{
  if (no_database_here) {
    /* OK */
  } else {
    fprintf(stderr, "Can't create database, it already exists!\n");
    exit(1);
  }
  return;

}
/*}}}*/
static int mark_nodes_done_since(struct links *x, time_t start, time_t end)/*{{{*/
{
  int flag_set = 0;
  struct node *y;

  for (y = x->next; y != (struct node *) x; y = y->chain.next) {
    if ((y->done >= start) && (y->done <= end)) {
      y->flag = 1;
      flag_set = 1;
    }

    if (has_kids(y)) {
      if (mark_nodes_done_since(&y->kids, start, end)) {
        y->flag = 1;
        flag_set = 1;
      }
    }
  }
  return flag_set;
}
/*}}}*/
static void print_report(struct links *x, int indent)/*{{{*/
{
  struct node *y;

  for (y = x->next; y != (struct node *) x; y = y->chain.next) {
    if (y->flag) {
      do_indent(indent);
      printf("%s\n", y->text);

      if (has_kids(y)) {
        print_report(&y->kids, indent + 3);
      }
    }
  }
}
/*}}}*/
static long read_interval(char *xx)/*{{{*/
{
  long interval;
  int nc;
  
  if (sscanf(xx, "%ld%n", &interval, &nc) != 1) {
    fprintf(stderr, "Cannot recognize interval '%s'\n", xx);
    exit(1);
  }
  
  xx += nc;
  switch (xx[0]) {
    case 'h': interval *= 3600; break;
    case 'd': interval *= 86400; break;
    case 'w': interval *= 7 * 86400; break;
    case 'm': interval *= 30 * 86400; break;
    case 'y': interval *= 365 * 86400; break;
    default:
      fprintf(stderr, "Can't understand interval multiplier '%s'\n", xx);
      exit(1);
  }
  return interval;
}
/*}}}*/
static void process_report(char **x)/*{{{*/
{
  long interval;
  time_t now, start, end;
  int argc;

  argc = count_args(x);
  start = end = now = time(NULL);

  switch (argc) {
    case 1:
      interval = read_interval(x[0]);
      start = now - interval;
      break;
    case 2:
      interval = read_interval(x[0]);
      start = now - interval;
      interval = read_interval(x[1]);
      end = now - interval;
      break;
    default:
      fprintf(stderr, "Usage: report <start_ago> [<end_ago>] (e.g. 3h, 5d, 1w etc)\n");
      break;
  }

  clear_flags(&top);
  mark_nodes_done_since(&top, start, end);
  print_report(&top, 0);
}
/*}}}*/
static void process_priority(char **x)/*{{{*/
{
  enum Priority priority = parse_priority(*x);
  struct node *n;
 
  while (*++x) {
    n = lookup_node(*x);
    n->priority = priority;
  }
}/*}}}*/

/* This will be variable eventually */
static char default_database_path[] = "./" DBNAME;

static char *get_database_path(int traverse_up)/*{{{*/
{
  char *env_var;
  env_var = getenv("TDL_DATABASE");
  if (env_var) {
    return env_var;
  } else {
    int at_root, size, dbname_len, found, stat_result;
    char *cwd, *result, *filename;
    struct stat statbuf;
    
    dbname_len = strlen(DBNAME);
    size = 1024;
    found = 0;
    at_root = 0;
    cwd = new_array(char, size);
    filename = new_array(char, size + dbname_len + 2);
    filename[0] = 0;
    do {
      result = getcwd(cwd, size);
      if (!result && (errno == ERANGE)) {
        size <<= 1;
        cwd = grow_array(char, size, cwd);
        filename = grow_array(char, size + dbname_len + 2, filename);
      } else {
        if (!strcmp(cwd, "/")) {
          at_root = 1;
        }
        strcpy(filename, cwd);
        strcat(filename, "/");
        strcat(filename, DBNAME);
        stat_result = stat(filename, &statbuf);
        if ((stat_result >= 0) && (statbuf.st_mode & 0600)) {
          found = 1;
          break;
        }

        if (!traverse_up) break;
        
        /* Otherwise, go up a level */
        chdir ("..");
      }
    } while (!at_root);

    free(cwd);
    if (found) {
      return filename;
    } else {
      return default_database_path;
    }
  }

}
/*}}}*/
static void rename_database(char *path)/*{{{*/
{
  int len;
  char *pathbak;

  len = strlen(path);
  pathbak = new_array(char, len + 5);
  strcpy(pathbak, path);
  strcat(pathbak, ".bak");
  if (rename(path, pathbak) < 0) {
    perror("warning, couldn't save backup database:");
  }
  free(pathbak);
  return;
} 
/*}}}*/
static char *executable_name(char *argv0)/*{{{*/
{
  char *p;
  for (p=argv0; *p; p++) ;
  for (; p>=argv0; p--) {
    if (*p == '/') return (p+1);
  }
  return argv0;
}
/*}}}*/
static void usage(void)/*{{{*/
{
  fprintf(stderr,
          "tdl, Copyright (C) 2001 Richard P. Curnow\n"
          "tdl comes with ABSOLUTELY NO WARRANTY.\n"
          "This is free software, and you are welcome to redistribute it\n"
          "under certain conditions; see the GNU General Public License for details.\n\n");

  fprintf(stderr,
          "tdl add [<parent_index>] [<priority>] <entry_text>\n"
          "tdla    [<parent_index>] [<priority>] <entry_text>\n"
          "   Add a new entry to the database\n\n"
          "tdl log [<parent_index>] [<priority>] <entry_text>\n"
          "   Add a new entry to the database, mark it done as well\n\n"
          "tdl list [-v] [-a]\n"
          "tdll     [-v] [-a]\n"
          "   List entries in database\n"
          "   -v : verbose (show dates, priorities etc)\n"
          "   -a : show all entries, including 'done' ones\n\n"
          "tdl done <entry_index> ...\n"
          "tdld     <entry_index> ...\n"
          "   Mark 1 or more entries as done\n\n"
          "tdl remove <entry_index> ...\n"
          "   Remove 1 or more entries from the database\n\n"
          "tdl edit <entry_index> <new_text>\n"
          "   Change the text of an entry\n\n"
          "tdl priority <new_priority> <entry_index> ...\n"
          "   Change the priority of 1 or more entries\n\n"
          "tdl report <start_ago> [<end_ago>]\n"
          "   Report completed tasks in interval (end defaults to now)\n\n"
          "tdl create\n"
          "   Create a new database in the current directory\n\n"
          "tdl help\n"
          "   Display this text\n\n"
          "<index>    : 1, 1.1 etc (see output of 'tdl list')\n"
          "<priority> : urgent|high|normal|low|verylow\n"
          "<ago>      : 1h, 3d, 1w, 2m, 3y (a digit + multiplier)\n"
          "<text>     : Any text (you'll need to quote it if >1 word)\n"
          );

}
/*}}}*/

/*{{{  int main (int argc, char **argv)*/
int main (int argc, char **argv)
{
  FILE *in, *out;
  char *database_path;
  int dirty = 0;
  int is_create_command;
  char *executable;
  int is_tdl;

  /* Initialise database */
  top.prev = (struct node *) &top;
  top.next = (struct node *) &top;

  executable = executable_name(argv[0]);
  is_tdl = (!strcmp(executable, "tdl"));

  /* Parse command line */
  if (is_tdl && (argc < 2)) {
    fprintf(stderr, "Need some stuff on command line\n");
    exit(1);
  }

  /*{{{  Get path to database */
  is_create_command = (is_tdl && !strcmp(argv[1], "create"));
  database_path = get_database_path(!is_create_command);
  /*}}}*/
  
  /*{{{  Load database*/
  in = fopen(database_path, "rb");
  if (in) {
    /* Database may not exist, e.g. if the program has never been run before.
       */
    read_database(in);
    fclose(in);
  } else {
    no_database_here = 1;
    if (!is_create_command) {
      fprintf(stderr, "warning: no database found above this directory\n");
    }
  }
  /*}}}*/

  if (is_tdl) {
    if (!strcmp(argv[1], "add")) {
      process_add(argv + 2, 0);
      dirty = 1;
    } else if (!strcmp(argv[1], "log")) {
      process_add(argv + 2, 1);
      dirty = 1;
    } else if (!strcmp(argv[1], "list")) {
      process_list(argv + 2);
    } else if (!strcmp(argv[1], "remove")) {
      process_remove(argv + 2);
      dirty = 1;
    } else if (!strcmp(argv[1], "done")) {
      process_done(argv + 2);
      dirty = 1;
    } else if (is_create_command) {
      process_create(argv + 2);
      dirty = 1;
    } else if (!strcmp(argv[1], "edit")) {
      process_edit(argv + 2);
      dirty = 1;
    } else if (!strcmp(argv[1], "report")) {
      process_report(argv + 2);
    } else if (!strncmp(argv[1], "pri", 3)) {
      process_priority(argv + 2);
      dirty = 1;
    } else if (!strcmp(argv[1], "help") ||
        !strcmp(argv[1], "usage") ||
        !strcmp(argv[1], "-h") ||
        !strcmp(argv[1], "--help")) {
      usage();
      exit(0);
    } else {
      fprintf(stderr, "Did not understand command line option\n");
      exit(1);
    }
  } else {
    if (!strcmp(executable, "tdla")) {
      process_add(argv + 1, 0);
      dirty = 1;
    } else if (!strcmp(executable, "tdll")) {
      process_list(argv + 1);
    } else if (!strcmp(executable, "tdld")) {
      process_done(argv + 1);
      dirty = 1;
    }
  }


  /*{{{  Save database */
  if (dirty) {
    if (!is_create_command) rename_database(database_path);
    out = fopen(database_path, "wb");
    if (!out) {
      fprintf(stderr, "Cannot open database %s for writing\n", database_path);
      exit(1);
    }

    write_database(out);
    fclose(out);
  }
  /*}}}*/
  
  return 0;

}
/*}}}*/

