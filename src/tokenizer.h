#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>

/**
 * @brief Splits a command line string into shell arguments.
 *        Handles whitespace, single quotes, double quotes, and backslash escaping.
 * @param arg (char *) Command line string to tokenize.
 * @param arg_size (size_t) Size of the command buffer.
 * @param argv (char *[]) Output array that stores pointers to each argument.
 * @param max_args (int) Maximum number of arguments argv can hold.
 * @return The number of parsed arguments.
 */
int tokenize_arg(char *arg, size_t arg_size, char *argv[], int max_args);

#endif
