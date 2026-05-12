#include "shell.h"
#include "trie.h"
#include "builtins.h"
#include "path.h"
#include <stdio.h>

int main(void) {
  setbuf(stdout, NULL);

  Trie *builtin_trie = trie_create();
  Trie *path_trie = trie_create();

  build_builtin_trie(builtin_trie);
  build_path_trie(path_trie);

  run_shell(builtin_trie, path_trie);

  trie_free(builtin_trie);
  trie_free(path_trie);

  return 0;
}
