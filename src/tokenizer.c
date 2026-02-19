#include "tokenizer.h"

#include <stdlib.h>
#include <ctype.h>

int tokenize_arg(char *arg, char *argv[], int max_args) {
  int argc = 0;
  char *p = arg;

  while (*p) {
    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p) break;
    if (argc >= max_args - 1) break; // Room for NULL

    char *start = p;  // Beginning of each token
    char *w = start;
    // Quote flags
    int is_sq = 0;
    int is_dq = 0;

    while (*p && (is_sq || is_dq || !isspace((unsigned char)*p))) {
      // Backslash handling
      if (*p == '\\') {
        if (!is_sq && !is_dq) { // Outside quotes
          p++;
          if (*p)
            *w++ = *p++;
        }
        else if (is_dq && !is_sq) { // Inside double quotes
          char next = *(p + 1);
          if (next == '\"' || next == '\\') {
            p++;
            *w++ = *p++;
          }
          else
            *w++ = *p++;
        }
        else  // Inside single quotes
          *w++ = *p++;
      }
      // Single quotes handling
      else if (*p == '\'' && !is_dq) {
        is_sq = !is_sq;
        p++;
      }
      // Double quotes handling
      else if (*p == '\"' && !is_sq) {
        is_dq = !is_dq;
        p++;
      }
      else {
        *w++ = *p++;
      }
    }
    // When stopped on whitespace, skip
    while (*p && isspace((unsigned char)*p)) p++;

    *w = '\0'; // End the token string
    argv[argc++] = start;
  }
  argv[argc] = NULL;
  return argc;
}
