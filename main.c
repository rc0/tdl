/*
   $Header: /cvs/src/tdl/main.c,v 1.17 2002/05/06 23:14:44 richard Exp $
  
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

#include "tdl.h"
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

/* The name of the database file (in whichever directory it may be) */
#define DBNAME ".tdldb"

/* Set if db doesn't exist in this directory */
static char *current_database_path = NULL;

/* The currently loaded database */
struct links top;

/* Flag for whether data is actually loaded yet */
static int is_loaded = 0;

/* Flag if currently loaded database has been changed and needs writing back to
 * the filesystem */
static int currently_dirty = 0;

/* Whether to complain about problems with file operations */
static int is_noisy = 1;

static void set_descendent_priority(struct node *x, enum Priority priority)/*{{{*/
{
  struct node *y;
  for (y = x->kids.next; y != (struct node *) &x->kids; y = y->chain.next) {
    y->priority = priority;
    set_descendent_priority(y, priority);
  }
}
/*}}}*/

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
    
    /* Reason for this : if using create in a subdirectory of a directory
     * already containing a .tdldb, the cwd after the call here from main would
     * get left pointing at the directory containing the .tdldb that already
     * exists, making the call here from process_create() fail.  So go back to
     * the directory where we started.  */
    chdir(orig_cwd);
    free(orig_cwd);

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
    if (is_noisy) {
      perror("warning, couldn't save backup database:");
    }
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
static void load_database(char *path) /*{{{*/
  /* Return 1 if successful, 0 if no database was found */
{
  FILE *in;
  currently_dirty = 0;
  in = fopen(path, "rb");
  if (in) {
    /* Database may not exist, e.g. if the program has never been run before.
       */
    read_database(in, &top);
    fclose(in);
    is_loaded = 1;
  } else {
    if (is_noisy) {
      fprintf(stderr, "warning: no database found above this directory\n");
    }
  }
}
/*}}}*/
static void save_database(char *path)/*{{{*/
{
  FILE *out;
  if (is_loaded && currently_dirty) {
    /* The next line only used to happen if the command wasn't 'create'.
     * However, it should quietly fail for create, where the existing database
     * doesn't exist */
    rename_database(path);
    out = fopen(path, "wb");
    if (!out) {
      fprintf(stderr, "Cannot open database %s for writing\n", path);
      exit(1);
    }
    write_database(out, &top);
    fclose(out);
  }
  currently_dirty = 0;
  return;
}
/*}}}*/
  
static void usage(void)/*{{{*/
{
  fprintf(stderr,
          "tdl, Copyright (C) 2001,2002 Richard P. Curnow\n"
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
          "tdl [-q] list [-v] [-a] [-m] [-1..9] [<parent_index>...]\n"
          "tdll [-q]     [-v] [-a] [-m] [-1..9] [<min-priority>] [<parent_index>...]\n"
          "   List entries in database (default from top node)\n"
          "   -v : verbose (show dates, priorities etc)\n"
          "   -a : show all entries, including 'done' ones\n"
          "   -m : don't use colours (monochrome)\n"
          "   -1,-2,..,-9 : summarise (and don't show) entries below this depth\n\n"
          "tdl [-q] done [@<datespec>] <entry_index>[...] ...\n"
          "tdld [-q]     [@<datespec>] <entry_index>[...] ...\n"
          "   Mark 1 or more entries as done\n\n"
          "tdl [-q] undo <entry_index>[...] ...\n"
          "   Mark 1 or more entries as not done (cancel effect of 'done')\n\n"
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
          "tdl [-q] edit [@<datespec>] <entry_index>[...] [<new_text>]\n"
          "   Change the text and/or start time of an entry\n\n"
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
          "tdl [-q] which\n"
          "   Display filename of database being used\n\n"
          "<index>    : 1, 1.1 etc (see output of 'tdl list')\n"
          "<priority> : urgent|high|normal|low|verylow\n"
          "<datespec> : [-|+][0-9]+[shdwmy][-hh[mm[ss]]]  OR\n"
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
  while (isspace(*p)) p++;
  if (*p == '$') {
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

static void process_create(char **x)/*{{{*/
{
  char *dbpath = get_database_path(0);
  struct stat sb;
  int result;

  result = stat(dbpath, &sb);
  if (result >= 0) {
    fprintf(stderr, "Can't create database <%s>, it already exists!\n", dbpath);
    exit(1);
  } else {
    /* Should have an empty database, and the dirty flag will be set */
    current_database_path = dbpath;
    /* don't emit complaint about not being able to move database to its backup */
    is_noisy = 0;
    /* Force empty database to be written out */
    is_loaded = currently_dirty = 1;
    return;
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
static void process_which(char **argv)/*{{{*/
{
  printf("%s\n", current_database_path);
  return;
}
/*}}}*/
static void process_version(char **x)/*{{{*/
{
  fprintf(stderr, "tdl %s\n", get_version());
  return;
}
/*}}}*/
static void process_exit(char **x)/*{{{*/
{
  save_database(current_database_path);
  exit(0);
}
/*}}}*/
static void process_quit(char **x)/*{{{*/
{
  /* Just get out quick, don't write the database back */
  exit(0);
}
/*}}}*/

/* All the main processing functions take an argv array and 0,1 or 2 integer
 * args */ 
typedef void (*fun2)(char **, int, int);
typedef void (*fun1)(char **, int);
typedef void (*fun0)(char **);

struct command {/*{{{*/
  char *name; /* add, remove etc */
  char *shortcut; /* tdla etc */
  void *func; /* function pointer */
  unsigned char  dirty; /* 1 if operation can dirty the database, 0 if read-only */
  unsigned char  nextra; /* how many integer args */
  unsigned char  i1;
  unsigned char  i2;    /* the integer args */
  unsigned char  load_db; /* 1 if cmd requires current database to be loaded first */
  unsigned char  interactive_ok; /* 1 if OK to use interactively. */
  unsigned char  non_interactive_ok; /* 1 if OK to use from command line */
};
/*}}}*/
struct command cmds[] = {/*{{{*/
  {"--help",   NULL,   (void *) usage,            0, 0, 0, 0, 0, 0, 1},
  {"-h",       NULL,   (void *) usage,            0, 0, 0, 0, 0, 0, 1},
  {"-V",       NULL,   (void *) process_version,  0, 0, 0, 0, 0, 0, 1},
  {"above",    NULL,   (void *) process_move,     1, 2, 0, 0, 1, 1, 1},
  {"add",      "tdla", (void *) process_add,      1, 1, 0, 0, 1, 1, 1},
  {"after",    NULL,   (void *) process_move,     1, 2, 1, 0, 1, 1, 1},
  {"below",    NULL,   (void *) process_move,     1, 2, 1, 0, 1, 1, 1},
  {"before",   NULL,   (void *) process_move,     1, 2, 0, 0, 1, 1, 1},
  {"create",   NULL,   (void *) process_create,   1, 0, 0, 0, 0, 0, 1},
  {"done",     "tdld", (void *) process_done,     1, 0, 0, 0, 1, 1, 1},
  {"edit",     NULL,   (void *) process_edit,     1, 0, 0, 0, 1, 1, 1},
  {"exit",     NULL,   (void *) process_exit,     0, 0, 0, 0, 0, 1, 0},
  {"export",   NULL,   (void *) process_export,   0, 0, 0, 0, 1, 1, 1},
  {"help",     NULL,   (void *) usage,            0, 0, 0, 0, 0, 1, 1},
  {"import",   NULL,   (void *) process_import,   1, 0, 0, 0, 1, 1, 1},
  {"into",     NULL,   (void *) process_move,     1, 2, 0, 1, 1, 1, 1},
  {"list",     "tdll", (void *) process_list,     0, 0, 0, 0, 1, 1, 1},
  {"log",      "tdlg", (void *) process_add,      1, 1, 1, 0, 1, 1, 1},
  {"priority", NULL,   (void *) process_priority, 1, 0, 0, 0, 1, 1, 1},
  {"purge",    NULL,   (void *) process_purge,    1, 0, 0, 0, 1, 1, 1},
  {"quit",     NULL,   (void *) process_quit,     0, 0, 0, 0, 0, 1, 0},
  {"remove",   NULL,   (void *) process_remove,   1, 0, 0, 0, 1, 1, 1},
  {"report",   NULL,   (void *) process_report,   0, 0, 0, 0, 1, 1, 1},
  {"undo",     NULL,   (void *) process_undo,     1, 0, 0, 0, 1, 1, 1},
  {"usage",    NULL,   (void *) usage,            0, 0, 0, 0, 0, 1, 1},
  {"version",  NULL,   (void *) process_version,  0, 0, 0, 0, 0, 1, 1},
  {"which",    NULL,   (void *) process_which,    0, 0, 0, 0, 0, 1, 1} /* FIXME */
};/*}}}*/

#define N(x) (sizeof(x) / sizeof(x[0]))

static int is_processing = 0;
static int was_signalled = 0;

static void handle_signal(int a)/*{{{*/
{
  was_signalled = 1;
  /* And close stdin, which should cause readline() in inter.c to return
   * immediately if it was active when the signal arrived. */
  close(0);
}
/*}}}*/
static void guarded_sigaction(int signum, struct sigaction *sa)/*{{{*/
{
  if (sigaction(signum, sa, NULL) < 0) {
    perror("sigaction");
    exit(1);
  }
}
/*}}}*/
static void setup_signals(void)/*{{{*/
{
  struct sigaction sa;
  if (sigemptyset(&sa.sa_mask) < 0) {
    perror("sigemptyset");
    exit(1);
  }
  sa.sa_handler = handle_signal;
  sa.sa_flags = 0;
  guarded_sigaction(SIGHUP, &sa);
  guarded_sigaction(SIGINT, &sa);
  guarded_sigaction(SIGQUIT, &sa);
  guarded_sigaction(SIGTERM, &sa);

  return;
}
/*}}}*/
void dispatch(char **argv, int is_interactive) /* and other args *//*{{{*/
{
  int i, n, index=-1;
  fun0 f0;
  fun1 f1;
  fun2 f2;
  char *executable;
  int is_tdl;
  char **p;

  if (was_signalled) {
    save_database(current_database_path);
    exit(0);
  }

  executable = executable_name(argv[0]);
  is_tdl = (!strcmp(executable, "tdl"));
  
  p = argv + 1;
  if (*p && !strcmp(*p, "-q")) p++;
  
  /* Parse command line */
  if (is_tdl && !*p) {
    /* If no arguments, go into interactive mode, but only if we didn't come from there (!) */
    if (!is_interactive) {
      setup_signals();
      interactive();
    }
    return;
  }
  
  n = N(cmds);
  if (is_tdl) {
    for (i=0; i<n; i++) {
      /* This is a crock - eventually, replace by a search that can hit at the shortest unambiguous substring */
      if ((is_interactive ? cmds[i].interactive_ok : cmds[i].non_interactive_ok) &&
          !strncmp(cmds[i].name, *p, 3)) {
        index = i;
        break;
      }
    }
  } else {
    for (i=0; i<n; i++) {
      /* This is a crock - eventually, replace by a search that can hit at the shortest unambiguous substring */
      if (cmds[i].shortcut && !strcmp(cmds[i].shortcut, executable)) {
        index = i;
        break;
      }
    }
  }

  if (index >= 0) {
    is_processing = 1;

    if (!is_loaded && cmds[index].load_db) {
      load_database(current_database_path);
    }

    switch (cmds[index].nextra) {
      case 0:
        f0 = (fun0) cmds[index].func;
        (*f0) (p + 1);
        break;
      case 1:
        f1 = (fun1) cmds[index].func;
        (*f1) (p + 1, cmds[index].i1);
        break;
      case 2:
        f2 = (fun2) cmds[index].func;
        (*f2) (p + 1, cmds[index].i1, cmds[index].i2);
        break;
    }
    if (cmds[index].dirty) {
      currently_dirty = 1;
    }

    is_processing = 0;
    if (was_signalled) {
      save_database(current_database_path);
      exit(0);
    }
    
  } else {
    fprintf(stderr, "Unknown command <%s>\n", argv[1]);
    if (!is_interactive) exit(1);
  }
  
}
/*}}}*/

/*{{{  int main (int argc, char **argv)*/
int main (int argc, char **argv)
{
  /* Initialise database */
  top.prev = (struct node *) &top;
  top.next = (struct node *) &top;

  if ((argc > 1) && (!strcmp(argv[1], "-q"))) {
    is_noisy = 0;
  }

  current_database_path = get_database_path(1);

  dispatch(argv, 0);

  save_database(current_database_path);
  return 0;
}
/*}}}*/

