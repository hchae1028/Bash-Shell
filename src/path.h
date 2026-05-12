#ifndef PATH_H
#define PATH_H

#include "trie.h"

/**
 * @brief Inserts all executable names in PATH into a Trie.
 * @param root (Trie *) Trie object used for PATH executable autocompletion.
 */
void build_path_trie(Trie *root);

#endif
