/*
   $Header: /cvs/src/tdl/main.c,v 1.10 2001/10/20 21:20:08 richard Exp $
  
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
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* The name of the database file (in whichever directory it may be) */
#define DBNAME ".tdldb"

/* The database */
struct links top;

/* Set if db doesn't exist in this directory */
static int no_database_here = 0;

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
static void set_descendent_priority(struct node *x, enum Priority priority)/*{{{*/
{
  struct node *y;
  for (y = x->kids.next; y != (struct node *) &x->kids; y = y->chain.next) {
    y->priority = priority;
    set_descendent_priority(y, priority);
  }
}
/*}}}*/
static void process_priority(char **x)/*{{{*/
{
  enum Priority priority = parse_priority(*x);
  struct node *n;
  int do_descendents;
 
  while (*++x) {
    do_descendents = include_descendents(*x); /* May modify *x */
    n = lookup_node(*x, 0, NULL);
    n->priority = priority;
    if (do_descendents) {
      set_descendent_priority(n, priority);
    }
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
    int at_root, orig_size, size, dbname_len, found, stat_result;
    char *orig_cwd, *cwd, *result, *filename;
    struct stat statbuf;
    
    dbname_len = strlen(DBNAME);
    size = 16;
    orig_size = 16;
    found = 0;
    at_root = 0;
    cwd = new_array(char, size);
    orig_cwd = new_array(char, orig_size);
    do {
      result = getcwd(orig_cwd, orig_size);
      if (!result) {
        if (errno == ERANGE) {
          orig_size <<= 1;
          orig_cwd = grow_array(char, orig_size, orig_cwd);
        } else {
          fprintf(stderr, "Unexpected error reading current directory\n");
          exit(1);
        }
      }
    } while (!result);
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
      free(orig_cwd);
      return filename;
    } else {
      chdir(orig_cwd);
      free(orig_cwd);
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
          "tdl [-q] add [@<datespec>] [<parent_index>] [<priority>] <entry_text>\n"
          "tdla [-q]    [@<datespec>] [<parent_index>] [<priority>] <entry_text>\n"
          "   Add a new entry to the database\n\n"
          "tdl [-q] log [@<datespec>] [<parent_index>] [<priority>] <entry_text>\n"
          "tdlg [-q]    [@<datespec>] [<parent_index>] [<priority>] <entry_text>\n"
          "   Add a new entry to the database, mark it done as well\n\n"
          "tdl [-q] list [-v] [-a] [<parent_index>...]\n"
          "tdll [-q]     [-v] [-a] [<min-priority>] [<parent_index>...]\n"
          "   List entries in database (default from top node)\n"
          "   -v : verbose (show dates, priorities etc)\n"
          "   -a : show all entries, including 'done' ones\n\n"
          "tdl [-q] done [@<datespec>] <entry_index>[...] ...\n"
          "tdld [-q]     [@<datespec>] <entry_index>[...] ...\n"
          "   Mark 1 or more entries as done\n\n"
          "tdl[-q]  remove <entry_index>[...] ...\n"
          "   Remove 1 or more entries from the database\n\n"
          "tdl [-q] above  <index_to_insert_above> <index_to_move> ...\n"
          "tdl [-q] before <index_to_insert_above> <index_to_move> ...\n"
          "   Move entries above another entry\n\n"
          "tdl [-q] below <index_to_insert_below> <index_to_move> ...\n"
          "tdl [-q] after <index_to_insert_below> <index_to_move> ...\n"
          "   Move entries below another entry\n\n"
          "tdl [-q] into <new_parent_index> <index_to_move> ...\n"
          "   Move entries to end of new parent\n\n"
          "tdl [-q] purge <since_datespec> [<ancestor_index> ...]\n"
          "   Remove old done entries in subtrees\n\n"
          "tdl [-q] edit <entry_index> <new_text>\n"
          "   Change the text of an entry\n\n"
          "tdl [-q] priority <new_priority> <entry_index> ...\n"
          "   Change the priority of 1 or more entries\n\n"
          "tdl [-q] report <start_datespec> [<end_datespec>]\n"
          "   Report completed tasks in interval (end defaults to now)\n\n"
          "tdl [-q] create\n"
          "   Create a new database in the current directory\n\n"
          "tdl [-q] import <filename>\n"
          "   Import entries from <filename>\n\n"
          "tdl [-q] export <filename> <entry_index> ...\n"
          "   Export entries to <filename>\n\n"
          "tdl [-q] help\n"
          "   Display this text\n\n"
          "tdl [-q] version\n"
          "   Display program version\n\n"
          "<index>    : 1, 1.1 etc (see output of 'tdl list')\n"
          "<priority> : urgent|high|normal|low|verylow\n"
          "<datespec> : [-|+][0-9]+[shdwmy]  OR\n"
          "             [-|+](sun|mon|tue|wed|thu|fri|sat)[-hh[mm[ss]]] OR\n"
          "             [[[cc]yy]mm]dd[-hh[mm[ss]]]\n"
          "<text>     : Any text (you'll need to quote it if >1 word)\n"
          );

}
/*}}}*/
static char *get_version(void)/*{{{*/
{
  static char buffer[256];
  static char cvs_version[] = "$Name:  $";
  char *p, *q;
  for (p=cvs_version; *p; p++) {
    if (*p == ':') {
      p++;
      break;
    }
  }
  if (!*p) {
    strcpy(buffer, "development version");
  } else {
    for (q=buffer; *p && *p != '$'; p++) {
      if (!isspace(*p)) {
        if (*p == '_') *q++ = '.';
        else *q++ = *p;
      }
    }
    *q = 0;
  }

  return buffer;
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
  int is_noisy = 1;

  /* Initialise database */
  top.prev = (struct node *) &top;
  top.next = (struct node *) &top;

  executable = executable_name(argv[0]);
  is_tdl = (!strcmp(executable, "tdl"));

  if ((argc > 1) && (!strcmp(argv[1], "-q"))) {
    is_noisy = 0;
    ++argv;
    --argc;
  }

  /* Parse command line */
  if (is_tdl && (argc < 2)) {
    fprintf(stderr, "Need a subcommand.  Try\n\n    tdl help\n\nfor further information.\n");
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
    read_database(in, &top);
    fclose(in);
  } else {
    no_database_here = 1;
    if (!is_create_command && is_noisy) {
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
    } else if (!strcmp(argv[1], "purge")) {
      process_purge(argv + 2);
      dirty = 1;
    } else if (!strcmp(argv[1], "remove")) {
      process_remove(argv + 2);
      dirty = 1;
    } else if (!strcmp(argv[1], "below") || !strcmp(argv[1], "after")) {
      process_move(argv + 2, 1, 0);
      dirty = 1;
    } else if (!strcmp(argv[1], "above") || !strcmp(argv[1], "before")) {
      process_move(argv + 2, 0, 0);
      dirty = 1;
    } else if (!strcmp(argv[1], "into")) {
      process_move(argv + 2, 0, 1);
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
    } else if (!strcmp(argv[1], "export")) {
      process_export(argv + 2);
    } else if (!strcmp(argv[1], "import")) {
      process_import(argv + 2);
      dirty = 1;
    } else if (!strcmp(argv[1], "help") ||
               !strcmp(argv[1], "usage") ||
               !strcmp(argv[1], "-h") ||
               !strcmp(argv[1], "--help")) {
      usage();
      exit(0);
    } else if (!strcmp(argv[1], "version") ||
               !strcmp(argv[1], "-V") ||
               !strcmp(argv[1], "--version")) {
      fprintf(stderr, "tdl %s\n", get_version());
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
    } else if (!strcmp(executable, "tdlg")) {
      process_add(argv + 1, 1);
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

    write_database(out, &top);
    fclose(out);
  }
  /*}}}*/
  
  return 0;

}
/*}}}*/

