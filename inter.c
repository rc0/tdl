/*
   $Header: /cvs/src/tdl/inter.c,v 1.2 2002/05/08 23:07:48 richard Exp $
  
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

/* This file deals with interactive mode */

#include "tdl.h"
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>

#ifdef USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

static int completion_entry_function(void)/*{{{*/
{
  return 0;
}
/*}}}*/
static char *generate_a_completion(char *text, int state)/*{{{*/
{
  static int list_index, len;
  char *name;

  if (!state) {
    list_index = 0;
    len = strlen(text);
  }

  while (list_index < n_cmds) {
    int inter_ok = cmds[list_index].interactive_ok;
    name = cmds[list_index].name;
    list_index++;
    if (inter_ok && !strncmp(name, text, len)) {
      return new_string(name);
    }
  }

  return NULL; /* For no matches */
}
/*}}}*/
static char **tdl_completion(char *text, int start, int end)/*{{{*/
{
  char **matches = NULL;
  if (start == 0) {
    matches = completion_matches(text, generate_a_completion);
  } else {
    int i;
  
    if (!strncmp(rl_line_buffer, "usage", 5) ||
        !strncmp(rl_line_buffer, "help", 4)) {

      matches = completion_matches(text, generate_a_completion);
      
    } else {
      for (i=0; i<n_cmds; i++) {
        if (!strncmp(rl_line_buffer, cmds[i].name, 3)) {
          if (cmds[i].synopsis) {
            fprintf(stderr, "\n%s %s\n", cmds[i].name, cmds[i].synopsis);
            rl_on_new_line();
          }
        }
      }
    }
  }

  return matches;
}
/*}}}*/
static void add_null_arg(char ***av, int *max, int *n)/*{{{*/
{
  if (*max == *n) {
    *max += 4;
    *av = grow_array(char *, *max, *av);
  }
  (*av)[*n] = NULL;
  ++*n;
  return;
}
/*}}}*/
static void add_arg(char ***av, int *max, int *n, char *p, int len)/*{{{*/
{
  char *u, *v;
  int nn;
  
  if (*max == *n) {
    *max += 4;
    *av = grow_array(char *, *max, *av);
  }
  
  (*av)[*n] = new_array(char, len + 1);
  /* Make copy of string, rejecting any escape characters (backslashes) */
  for (u=p, v=(*av)[*n], nn=len; nn>0; ) {
    if (*u == '\\') {
      u++, nn--;
      switch(*u) {
        case 'n':
          *v++ = '\n';
          u++, nn--;
          break;
        case 't':
          *v++ = '\t';
          u++, nn--;
          break;
        default:
          *v++ = *u++;
          nn--;
          break;
      }
    } else {
      *v++ = *u++;
      nn--;
    }
  }
  *v = 0;
  ++*n;
}
/*}}}*/
static void split_line_and_dispatch(char *line)/*{{{*/
{
  static char **argv = NULL;
  static int max = 0;
  int i, n;
  char *av0 = "tdl";
  char *p, *q, t;

  /* Poke in argv[0] */
  n = 0;
  add_arg(&argv, &max, &n, av0, strlen(av0));

  /* Now, split the line and add each argument */
  p = line;
  for (;;) {
    while (*p && isspace(*p)) p++;
    if (!*p) break;

    /* p points to start of argument. */
    if (*p == '\'' || *p == '"') {
      t = *p;
      p++;
      /* Scan for matching terminator */
      q = p + 1;
      while (*q && (*q != t)) {
        if (*q == '\\') q++; /* Escape following character */
        if (*q) q++; /* Bogus backslash at end of line */
      }
    } else {
      /* Single word arg */
      q = p + 1;
      while (*q && !isspace(*q)) q++;
    }
    add_arg(&argv, &max, &n, p, q - p);
    if (!*q) break;
    p = q + 1;
  }
  
  add_null_arg(&argv, &max, &n);
#ifdef TEST
  /* For now, print arg list for debugging */
  {
    int i;
    for (i=0; argv[i]; i++) {
      printf("arg %d : <%s>\n", i, argv[i]);
    }
    fflush(stdout);
  }
#else
  /* Now dispatch */
  dispatch(argv, 1);
#endif

  /* Now free arguments */
  for (i=0; argv[i]; i++) {
    free(argv[i]);
  }
  
  return;
}
/*}}}*/
#ifdef USE_READLINE
static void interactive_readline(void)/*{{{*/
{
  char *cmd;
  char *p;
  int had_char;
  
  do {
    cmd = readline("tdl> ");

    /* At end of file (e.g. input is a script, or user hits ^D, or stdin (file 
       #0) is closed by the signal handler) */
    if (!cmd) return;
    
    /* Check if line is blank */
    for (p=cmd, had_char=0; *p; p++) {
      if (!isspace(*p)) {
        had_char = 1;
        break;
      }
    }

    if (had_char) {
      add_history(cmd);
      split_line_and_dispatch(cmd);
      free(cmd);
    }
  } while (1);

  return;
      
}
/*}}}*/
#endif
static void interactive_stdio(void)/*{{{*/
{
  int n, max = 0;
  char *line = NULL;
  int c;
  int at_eof;
  
  do {
    printf("tdl> ");
    fflush(stdout);
    n = 0;
    at_eof = 0;
    do {
      c = getchar();
      if (c == EOF) {
        at_eof = 1;
        break;
      } else {
        if (n == max) {
          max += 1024;
          line = grow_array(char, max, line);
        }
        if (c != '\n') {
          line[n++] = c;
        } else {
          line[n++] = 0;
          split_line_and_dispatch(line);
        }
      }
    } while (c != '\n');
  } while (!at_eof);
}
/*}}}*/

void interactive(void)/*{{{*/
{
  /* Main interactive function */
#ifdef USE_READLINE
  if (isatty(0)) {
    rl_completion_entry_function = completion_entry_function;
    rl_attempted_completion_function = (CPPFunction *) tdl_completion;
    interactive_readline();
  } else {
    /* In case someone wants to drive tdl from a script, by redirecting stdin to it. */
    interactive_stdio();
  }
#else
  interactive_stdio();
#endif
  return;
}

/*}}}*/

/*{{{  Main routine [for testing]*/
#ifdef TEST
int main (int argc, char **argv) {
  interactive();
  return 0;
}
#endif
/*}}}*/
