#include "tokenizer.h"
#include <stdlib.h>
#include <ctype.h>

typedef struct {
  char *p;    // read pointer
  char *w;    // write pointer
  int is_sq;  // is in single quotes
  int is_dq;  // is in double quotes
  int stopped_on_pipe;  // Pipeline '|' character
} Token;

/**
 * @brief Moves the read pointer past any whitespace characters.
 * @param t (Token *) Token state containing the current read pointer.
 */
static void skip_whitespace(Token *t) {
  while (*t->p && isspace((unsigned char)*t->p))
    t->p++;
}

/**
 * @brief Handles backslash escaping while parsing a token.
 *        Backslashes are treated differently outside quotes,
 *        inside double quotes, and inside single quotes.
 * @param t (Token *) Token state containing quote flags and read/write pointers.
 */
static void handle_backslash(Token *t) {
  if (!t->is_sq && !t->is_dq) {   // Outside quotes
    t->p++;
    if (*t->p)
      *t->w++ = *t->p++;
  }
  else if (t->is_dq && !t->is_sq) { // Inside double quotes
    char next = *(t->p + 1);
    if (next == '\"' || next == '\\') {
      t->p++;
      *t->w++ = *t->p++;
    }
    else
      *t->w++ = *t->p++;
  }
  else  // Inside single quotes
    *t->w++ = *t->p++;
}

/**
 * @brief Handles one character while parsing a token.
 *        Updates quote state, processes backslashes, or copies normal characters.
 * @param t (Token *) Token state containing quote flags and read/write pointers.
 */
static void handle_token(Token *t) {
  if (*t->p == '|' && !t->is_sq && !t->is_dq)
    t->stopped_on_pipe = 1;
  else if (*t->p == '\\')
    handle_backslash(t);
  else if (*t->p == '\'' && !t->is_dq) {  // Handle single quotes
    t->is_sq = !t->is_sq;
    t->p++;
  }
  else if (*t->p == '\"' && !t->is_sq) {  // Handle double quotes
    t->is_dq = !t->is_dq;
    t->p++;
  }
  else
    *t->w++ = *t->p++;
}

/**
 * @brief Splits a command line string into shell arguments.
 *        Handles whitespace, single quotes, double quotes, and backslash escaping.
 *        Returns the number of parsed arguments.
 * @param arg (char *) Command line string to tokenize.
 * @param argv (char *[]) Output array that stores pointers to each argument.
 * @param max_args (int) Maximum number of arguments argv can hold.
 */
int tokenize_arg(char *arg, char *argv[], int max_args) {
  int argc = 0;
  Token t = {.p = arg,
             .w = arg,
             .is_sq = 0,
             .is_dq = 0,
             .stopped_on_pipe = 0};

  while (*t.p) {
    skip_whitespace(&t);
    if (!*t.p) break;
    if (argc >= max_args - 1) break; // Room for NULL

    char *start = t.p;
    t.w = start;
    t.is_sq = 0;
    t.is_dq = 0;
    t.stopped_on_pipe = 0;

    while (*t.p
           && (t.is_sq || t.is_dq || !isspace((unsigned char)*t.p))) {
      handle_token(&t);
      if (t.stopped_on_pipe)
        break;
    }

    if (t.stopped_on_pipe && start == t.p) {
      argv[argc++] = "|";
      t.p++;
      continue;
    }

    if (!t.stopped_on_pipe)
      skip_whitespace(&t);

    *t.w = '\0'; // End the token string
    argv[argc++] = start;

    if (t.stopped_on_pipe) {
      if (argc >= max_args - 1)
        break;

      argv[argc++] = "|";
      t.p++;
    }
  }
  argv[argc] = NULL;
  return argc;
}
