#ifndef PIPELINE_H
#define PIPELINE_H

#include "redirection.h"

void execute_pipeline(char *argv[], Redirection *redir);

/**
 * @brief Checks whether a given list of arguments has a pipeline command "|".
 *        Returns the index of the pipeline command, -1 if not found.
 * @param argv (char *[]) Tokenized command argument list.
 */
int find_pipeline_index(char *argv[]);

#endif
