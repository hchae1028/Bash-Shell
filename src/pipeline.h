#ifndef PIPELINE_H
#define PIPELINE_H

#define MAX_PIPELINE 16

void execute_pipeline(char *argv[]);

/**
 * @brief Checks whether a given list of arguments has a pipeline command "|".
 * @param argv (char *[]) Tokenized command argument list.
 * @return Index of the pipeline operator, or -1 if not found.
 */
int find_pipeline_index(char *argv[]);

#endif
