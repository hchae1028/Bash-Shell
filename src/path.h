#ifndef PATH_H
#define PATH_H

#include "trie.h"

/**
 * @brief Inserts all executable names in PATH into a Trie.
 * @param root (Trie *) Trie object used for PATH executable autocompletion.
 */
void build_path_trie(Trie *root);

/**
 * @brief Searches PATH for a given command argument.
 *        Stores the full executable path in pathbuf if found.
 *        Returns 1 if the command is found, 0 otherwise.
 * @param pathbuf (char *) Buffer used to store the full executable path.
 * @param pathbuf_size (size_t) Size of the path buffer.
 * @param arg (const char *) Command name to search for.
 */
int parse_path(char *pathbuf, size_t pathbuf_size, const char *arg);

#endif
