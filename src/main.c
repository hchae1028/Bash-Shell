#include "shell.h"
#include <stdio.h>

int main(void) {
  setbuf(stdout, NULL);
  run_shell();

  return 0;
}
