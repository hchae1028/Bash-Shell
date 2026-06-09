#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "redirection.h"

/**
 * @brief Runs an external program using posix_spawnp.
 *        Waits for the child process to finish before returning.
 * @param argv (char *[]) Argument list where argv[0] is the command name.
 * @param redir (Redirection *) Stores info about stdout/stderr redirection filenames and whether to append.
 * @return 0 if the program starts successfully, nonzero otherwise.
 */
int run_program(char *argv[], Redirection *redir);

#endif
