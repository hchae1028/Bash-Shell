#include "shell.h"
#include "trie.h"
#include "builtins.h"
#include <stdio.h>

int main(void) {
  setbuf(stdout, NULL);

  Trie *builtin_trie = trie_create();
  build_builtin_trie(builtin_trie);

  run_shell(builtin_trie);

  trie_free(builtin_trie);

  return 0;
}
