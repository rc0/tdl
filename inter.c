/*
   $Header: /cvs/src/tdl/inter.c,v 1.13 2003/05/14 23:31:51 richard Exp $
  
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
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>

#ifdef USE_READLINE
#if BARE_READLINE_H 
#include <readline.h>
#include <history.h>
#else
#include <readline/readline.h>
#include <readline/history.h>
#endif

#if USE_RL_COMPLETION_MATCHES
#define COMPLETION_MATCHES rl_completion_matches
#else
#define COMPLETION_MATCHES completion_matches
#endif

#endif


#ifdef USE_READLINE
static char *generate_a_command_completion(const char *text, int state)/*{{{*/
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
static char *generate_a_priority_completion(const char *text, int state)/*{{{*/
{
  static char *priorities[] = {"urgent", "high", "normal", "low", "verylow", NULL};
  static int list_index, len;
  char *name;

  if (!state) {
    list_index = 0;
    len = strlen(text);
  }

  while ((name = priorities[list_index])) {
    list_index++;
    if (!strncmp(name, text, len)) {
      return new_string(name);
    }
  }

  return NULL; /* For no matches */
}
/*}}}*/

/* Structure to build a single linked list of nodes, working towards the
 * ancestor. */
struct node_chain {
  struct node_chain *next;
  struct node *node;
  struct node *start; /* To see if we've got back to the start of the ring */
  int index;
};

static struct node_chain *start_chain(struct links *x, struct node_chain *parent)/*{{{*/
{
  struct node_chain *result;
  result = new(struct node_chain);
  result->start = (struct node *) x;
  result->node = x->next;
  result->index = 1;
  result->next = parent;
  return result;
}
/*}}}*/
static void cleanup_chain(struct node_chain *chain)/*{{{*/
{
  /* TODO: Reclaim memory */
  return;
}
/*}}}*/
static char *format_index(struct node_chain *chain)/*{{{*/
{
  int indices[256];
  int n, first;
  struct node_chain *x;
  char buf[1024], buf1[32];

  for (n=0, x=chain; x; n++, x=x->next) {
    indices[n] = x->index;
  }
  buf[0] = '\0';
  first = 1;
  while (--n >= 0) {
    sprintf(buf1, "%d", indices[n]);
    if (!first) {
      strcat(buf, ".");
    } else {
      first = 0;
    }
    strcat (buf, buf1);
  }
  return new_string(buf);
}
/*}}}*/
static struct node *advance_chain(struct node_chain **pchain, char **index_string)/*{{{*/
{
  struct node_chain *chain = *pchain;
  struct node *result = NULL;

#if DEBUG_ADVANCE
  fprintf(stderr, "advance chain, top index=%d\n", chain->index);
#endif
  
  while (1) {
    if (chain->node == chain->start) {
      struct node_chain *next = chain->next;
      free(chain);
      chain = next;
      if (chain) {
        chain->node = chain->node->chain.next;
        ++chain->index;
#if DEBUG_ADVANCE
        fprintf(stderr, "Returning to outer level, index=%d\n", chain->index);
#endif
        continue;
      } else {
#if DEBUG_ADVANCE
        fprintf(stderr, "Got to outermost level\n");
#endif
        result = NULL;
        break;
      }
    } else if (has_kids(chain->node)) {
#if DEBUG_ADVANCE
      fprintf(stderr, "Node has children, scanning children next\n");
#endif
      result = chain->node;
      *index_string = format_index(chain);
      /* Set-up to visit kids next time */
      chain = start_chain(&chain->node->kids, chain);
      break;
    } else {
      /* Ordinary case */
#if DEBUG_ADVANCE
      fprintf(stderr, "ordinary case\n");
#endif
      result = chain->node;
      *index_string = format_index(chain);
      chain->node = chain->node->chain.next;
      ++chain->index;
      break;
    }
  }

  *pchain = chain;
  return result;
}
/*}}}*/
static char *generate_a_done_completion(const char *text, int state)/*{{{*/
{
  static struct node_chain *chain = NULL;
  struct node *node;
  struct node *narrow_top;
  char *buf;
  int len;

  load_database_if_not_loaded();

  if (!state) {
    /* Re-initialise the node chain */
    if (chain) cleanup_chain(chain);
    narrow_top = get_narrow_top();
    chain = start_chain((narrow_top ? &narrow_top->kids : &top), NULL);
  }

  len = strlen(text);
  while ((node = advance_chain(&chain, &buf))) {
    int undone = (!node->done);
    if (undone && !strncmp(text, buf, len)) {
      return buf;
    } else {
      /* Avoid gross memory leak */
      free(buf);
    }
  }
  return NULL;
}
/*}}}*/

/* Can't pass extra args thru to completers :-( */
static int want_postponed_entries = 0;

static char *generate_a_postpone_completion(const char *text, int state)/*{{{*/
{
  static struct node_chain *chain = NULL;
  struct node *node;
  struct node *narrow_top;
  char *buf;
  int len;

  load_database_if_not_loaded();

  if (!state) {
    /* Re-initialise the node chain */
    if (chain) cleanup_chain(chain);
    narrow_top = get_narrow_top();
    chain = start_chain((narrow_top ? &narrow_top->kids : &top), NULL);
  }

  len = strlen(text);
  while ((node = advance_chain(&chain, &buf))) {
    int is_done = (node->done > 0);
    int not_postponed = (node->arrived != POSTPONED_TIME);
    if (!is_done && (not_postponed ^ want_postponed_entries) && !strncmp(text, buf, len)) {
      return buf;
    } else {
      /* Avoid gross memory leak */
      free(buf);
    }
  }
  return NULL;
}
/*}}}*/

char **complete_help(char *text, int index)/*{{{*/
{
  char **matches;
  matches = COMPLETION_MATCHES(text, generate_a_command_completion);
  return matches;
}
/*}}}*/
char **default_completer(char *text, int index)/*{{{*/
{
  if (cmds[index].synopsis) {
    fprintf(stderr, "\n%s %s\n", cmds[index].name, cmds[index].synopsis);
    rl_on_new_line();
  }
  return NULL;
}
/*}}}*/
char **complete_list(char *text, int index)/*{{{*/
{
  char **matches;
  if (text[0] && isalpha(text[0])) {
    /* Try to complete priority */
    matches = COMPLETION_MATCHES(text, generate_a_priority_completion);
    return matches;
  } else {
    return default_completer(text, index);
  }
}
/*}}}*/
char **complete_priority(char *text, int index)/*{{{*/
{
  return complete_list(text, index);
}
/*}}}*/
char **complete_postpone(char *text, int index)/*{{{*/
{
  char **matches;
  want_postponed_entries = 0;
  matches = COMPLETION_MATCHES(text, generate_a_postpone_completion);
  return matches;
}
/*}}}*/
char **complete_open(char *text, int index)/*{{{*/
{
  char **matches;
  want_postponed_entries = 1;
  matches = COMPLETION_MATCHES(text, generate_a_postpone_completion);
  return matches;
}
/*}}}*/
char **complete_done(char *text, int index)/*{{{*/
{
  char **matches;
  matches = COMPLETION_MATCHES(text, generate_a_done_completion);
  return matches;
}
/*}}}*/

static char **tdl_completion(char *text, int start, int end)/*{{{*/
{
  char **matches = NULL;
  if (start == 0) {
    matches = COMPLETION_MATCHES(text, generate_a_command_completion);
  } else {
    int i;
  
    for (i=0; i<n_cmds; i++) {
      if (!strncmp(rl_line_buffer, cmds[i].name, cmds[i].matchlen)) {
        if (cmds[i].completer) {
          matches = (cmds[i].completer)(text, i);
        } else {
          matches = default_completer(text, i);
        }
        break;
      }
    }
  }

  return matches;
}
/*}}}*/
static char **null_tdl_completion(char *text, int start, int end)/*{{{*/
{
  return NULL;
}
/*}}}*/
#else
char **complete_help(char *text, int index)/*{{{*/
{
  return NULL;
}/*}}}*/
char **complete_list(char *text, int index)/*{{{*/
{
  return NULL;
}/*}}}*/
char **complete_priority(char *text, int index)/*{{{*/
{
  return NULL;
}/*}}}*/
char **complete_postponed(char *text, int index)/*{{{*/
{
  return NULL;
}
/*}}}*/
char **complete_open(char *text, int index)/*{{{*/
{
  return NULL;
}
/*}}}*/
char **complete_done(char *text, int index)/*{{{*/
{
  return NULL;
}
/*}}}*/
#endif

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

static char **argv = NULL;

static void split_line_and_dispatch(char *line)/*{{{*/
{
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
  dispatch(argv);
#endif

  /* Now free arguments */
  for (i=0; argv[i]; i++) {
    free(argv[i]);
  }
  
  return;
}
/*}}}*/
static int is_line_blank(char *line)/*{{{*/
{
  char *p;
  for (p=line; *p; p++) {
    if (!isspace(*p)) return 0;
  }
  return 1;
}
/*}}}*/
static char *make_prompt(void)/*{{{*/
{
  char *narrow_prefix = get_narrow_prefix();
  char *result;
  if (narrow_prefix) {
    int length;
    length = strlen(narrow_prefix) + 8;
    result = new_array(char, length);
    strcpy(result, "tdl[");
    strcat(result, narrow_prefix);
    strcat(result, "]> ");
  } else {
    result = new_string("tdl> ");
  }
  return result;
}
/*}}}*/
#ifdef USE_READLINE
static void interactive_readline(void)/*{{{*/
{
  char *cmd;
  int had_char;
  
  do {
    char *prompt = make_prompt();
    cmd = readline(prompt);
    free(prompt);

    /* At end of file (e.g. input is a script, or user hits ^D, or stdin (file 
       #0) is closed by the signal handler) */
    if (!cmd) return;
    
    /* Check if line is blank */
    had_char = !is_line_blank(cmd);

    if (had_char) {
      add_history(cmd);
      split_line_and_dispatch(cmd);
      free(cmd);
    }
  } while (1);

  return;
      
}
/*}}}*/

static char *readline_initval = NULL;
static int setup_initval_hook(void)/*{{{*/
{
  if (readline_initval) {
    rl_insert_text(readline_initval);
    rl_redisplay();
  }
  return 0;
}
/*}}}*/
static char *interactive_text_readline(char *prompt, char *initval, int *is_blank, int *error)/*{{{*/
{
  char *line;
  Function *old_rl_pre_input_hook = NULL;
  
  *error = 0;
  old_rl_pre_input_hook = rl_pre_input_hook;
  if (initval) {
    readline_initval = initval;
    rl_pre_input_hook = setup_initval_hook;
  }
  line = readline(prompt);
  rl_pre_input_hook = old_rl_pre_input_hook;
  readline_initval = NULL;
  *is_blank = line ? is_line_blank(line) : 1;
  if (line && !*is_blank) add_history(line);
  return line;
}
/*}}}*/
#endif
static char *get_line_stdio(char *prompt, int *at_eof)/*{{{*/
{
  int n, max = 0;
  char *line = NULL;
  int c;
  
  *at_eof = 0;
  printf("%s",prompt);
  fflush(stdout);
  n = 0;
  do {
    c = getchar();
    if (c == EOF) {
      *at_eof = 1;
      return NULL;
    } else {
      if (n == max) {
        max += 1024;
        line = grow_array(char, max, line);
      }
      if (c != '\n') {
        line[n++] = c;
      } else {
        line[n++] = 0;
        return line;
      }
    }
  } while (c != '\n');
  assert(0);
}
/*}}}*/
static void interactive_stdio(void)/*{{{*/
{
  char *line = NULL;
  int at_eof;
  
  do {
    line = get_line_stdio("tdl> ", &at_eof);
    if (!at_eof) {
      split_line_and_dispatch(line);
      free(line);
    }
  } while (!at_eof);
}
/*}}}*/
static char *interactive_text_stdio(char *prompt, char *initval, int *is_blank, int *error)/*{{{*/
{
  char *line;
  int at_eof;
  *error = 0;
  line = get_line_stdio(prompt, &at_eof);
  *is_blank = line ? is_line_blank(line) : 0;
  return line;
}
/*}}}*/
char *interactive_text (char *prompt, char *initval, int *is_blank, int *error)/*{{{*/
{
#ifdef USE_READLINE
  if (isatty(0)) {
    char *result;
    rl_attempted_completion_function = (CPPFunction *) null_tdl_completion;
    result = interactive_text_readline(prompt, initval, is_blank, error);
    rl_attempted_completion_function = (CPPFunction *) tdl_completion;
    return result;
  } else {
    /* In case someone wants to drive tdl from a script, by redirecting stdin to it. */
    return interactive_text_stdio(prompt, initval, is_blank, error);
  }
#else
  return interactive_text_stdio(prompt, initval, is_blank, error);
#endif
}
/*}}}*/

void interactive(void)/*{{{*/
{
  /* Main interactive function */
#ifdef USE_READLINE
  if (isatty(0)) {
    rl_completion_entry_function = NULL;
    rl_attempted_completion_function = (CPPFunction *) tdl_completion;
    interactive_readline();
  } else {
    /* In case someone wants to drive tdl from a script, by redirecting stdin to it. */
    interactive_stdio();
  }
#else
  interactive_stdio();
#endif
  if (argv) free(argv);
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
