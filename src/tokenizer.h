#ifndef TOKENIZER_H
#define TOKENIZER_H

/**
 * @brief Splits a command line string into shell arguments.
 *        Handles whitespace, single quotes, double quotes, and backslash escaping.
 *        Returns the number of parsed arguments.
 * @param arg (char *) Command line string to tokenize.
 * @param argv (char *[]) Output array that stores pointers to each argument.
 * @param max_args (int) Maximum number of arguments argv can hold.
 */
int tokenize_arg(char *arg, char *argv[], int max_args);

#endif
