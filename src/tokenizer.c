#include "tokenizer.h"
#include "shell.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  const char *p;       // read pointer
  char *w;             // write pointer
  char *end;           // one past the end of the write buffer
  int is_sq;           // is in single quotes
  int is_dq;           // is in double quotes
  int stopped_on_pipe; // Pipeline '|' character
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
 * @brief Writes one character to the token output buffer.
 *        Keeps one byte available for the token's null terminator.
 * @param t (Token *) Token state containing the output buffer.
 * @param c (char) Character to write.
 * @return 1 on success, 0 otherwise.
 */
static int write_char(Token *t, char c) {
  if (t->w + 1 >= t->end)
    return 0;

  *t->w++ = c;
  return 1;
}

/**
 * @brief Ends the current token and advances the write pointer.
 * @param t (Token *) Token state containing the output buffer.
 * @return 1 on success, 0 otherwise.
 */
static int end_token(Token *t) {
  if (t->w >= t->end)
    return 0;

  *t->w++ = '\0';
  return 1;
}

/**
 * @brief Handles backslash escaping while parsing a token.
 *        Backslashes are treated differently outside quotes,
 *        inside double quotes, and inside single quotes.
 * @param t (Token *) Token state containing quote flags and read/write pointers.
 * @return 1 on success, 0 otherwise.
 */
static int handle_backslash(Token *t) {
  if (!t->is_sq && !t->is_dq) {   // Outside quotes
    t->p++;
    if (*t->p)
      return write_char(t, *t->p++);
    return write_char(t, '\\');
  }
  else if (t->is_dq && !t->is_sq) { // Inside double quotes
    char next = *(t->p + 1);
    if (next == '\"' || next == '\\' || next == '$') {
      t->p++;
      return write_char(t, *t->p++);
    }
    else
      return write_char(t, *t->p++);
  }
  else  // Inside single quotes
    return write_char(t, *t->p++);
}

static int is_var_start(char c) {
  return (isalpha((unsigned char)c) || c == '_');
}

static int is_var_char(char c) {
  return (isalnum((unsigned char)c) || c == '_');
}

/**
 * @brief Expands one environment variable into the current token.
 *        If '$' is not followed by a valid variable name, it stays literal.
 * @param t (Token *) Token state containing read/write pointers.
 * @return 1 on success, 0 otherwise.
 */
static int handle_dollar(Token *t) {
  char name[32];
  size_t name_len = 0;
  const char *value;

  t->p++; // Skip '$'

  if (!is_var_start(*t->p))
    return write_char(t, '$');

  while (*t->p && is_var_char(*t->p)) {
    if (name_len + 1 >= sizeof(name))
      return 0;

    name[name_len++] = *t->p++;
  }
  name[name_len] = '\0';

  value = getenv(name);
  if (value == NULL)
    value = "";

  for (size_t i = 0; value[i] != '\0'; i++) {
    if (!write_char(t, value[i]))
      return 0;
  }

  return 1;
}

/**
 * @brief Handles one character while parsing a token.
 *        Updates quote state, processes backslashes, or copies normal characters.
 * @param t (Token *) Token state containing quote flags and read/write pointers.
 * @return 1 on success, 0 otherwise.
 */
static int handle_token(Token *t) {
  if (*t->p == '|' && !t->is_sq && !t->is_dq)
    t->stopped_on_pipe = 1;
  else if (*t->p == '\\')
    return handle_backslash(t);
  else if (*t->p == '\'' && !t->is_dq) {  // Handle single quotes
    t->is_sq = !t->is_sq;
    t->p++;
  }
  else if (*t->p == '\"' && !t->is_sq) {  // Handle double quotes
    t->is_dq = !t->is_dq;
    t->p++;
  }
  else if (*t->p == '$' && !t->is_sq)
    return handle_dollar(t);
  else
    return write_char(t, *t->p++);

  return 1;
}

/**
 * @brief Splits a command line string into shell arguments.
 *        Handles whitespace, single quotes, double quotes, and backslash escaping.
 *        Returns the number of parsed arguments.
 * @param arg (char *) Command line string to tokenize.
 * @param arg_size (size_t) Size of the arg buffer.
 * @param argv (char *[]) Output array that stores pointers to each argument.
 * @param max_args (int) Maximum number of arguments argv can hold.
 * @return The number of parsed arguments.
 */
int tokenize_arg(char *arg, size_t arg_size, char *argv[], int max_args) {
  int argc = 0;
  char input[COMMAND_SIZE];

  if (arg_size == 0 || max_args <= 0)
    return -1;

  strncpy(input, arg, sizeof(input) - 1);
  input[sizeof(input) - 1] = '\0';

  Token t = {.p = input,
             .w = arg,
             .end = arg + arg_size,
             .is_sq = 0,
             .is_dq = 0,
             .stopped_on_pipe = 0};

  while (*t.p) {
    skip_whitespace(&t);
    if (!*t.p) break;
    if (argc >= max_args - 1) break; // Room for NULL

    char *start = t.w;
    t.is_sq = 0;
    t.is_dq = 0;
    t.stopped_on_pipe = 0;

    while (*t.p
           && (t.is_sq || t.is_dq || !isspace((unsigned char)*t.p))) {
      if (!handle_token(&t))
        return -1;
      if (t.stopped_on_pipe)
        break;
    }

    if (t.stopped_on_pipe && start == t.w) {
      if (!write_char(&t, '|') || !end_token(&t))
        return -1;

      argv[argc++] = start;
      t.p++;
      continue;
    }

    if (!end_token(&t))
      return -1;

    argv[argc++] = start;

    if (t.stopped_on_pipe) {
      if (argc >= max_args - 1)
        break;

      start = t.w;
      if (!write_char(&t, '|') || !end_token(&t))
        return -1;

      argv[argc++] = start;
      t.p++;
    }
  }
  argv[argc] = NULL;
  return argc;
}
