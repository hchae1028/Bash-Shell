#ifndef EXPANSION_H
#define EXPANSION_H

#include <stdlib.h>

int expand_env_var(const char *input, char *output, size_t expanded_size);

#endif
