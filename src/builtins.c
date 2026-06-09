#include "builtins.h"
#include "shell.h"
#include "trie.h"
#include "path.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char *builtins[] = {"echo", "type", "exit", "cd", "printf", "pwd", ":", ".", "break", "history",
                        "continue", "eval", "exec", "export", "true", "false", "getopts", "hash",
                        "readonly", "return", "shift", "test", "trap", "times", "umask", "unset",
                        "alias", "bind", "builtin", "caller", "command", "declare", "enable", "let",
                        "logout", "local", "mapfile", "read", "readarray", "source", "typeset",
                        "ulimit", "unalias"};

/**
 * @brief Checks whether a given command argument is a shell builtin.
 * @param arg (const char *) Command line argument to be checked.
 * @return 1 if arg is a shell builtin, 0 otherwise.
 */
int is_builtin(const char *arg) {
  for (size_t i = 0; i < sizeof(builtins)/sizeof(builtins[0]); i++) {
    if (strcmp(arg, builtins[i]) == 0)
      return 1;
  }
  return 0;
}

/**
 * @brief Handles the echo builtin command.
 *        Prints each argument after the command name, separated by spaces.
 * @param argc (int) Number of command line arguments.
 * @param argv (char *[]) Argument list passed to echo.
 */
void builtin_echo(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    if (i > 1)
      printf(" ");
    printf("%s", argv[i]);
  }
  printf("\n");
}

/**
 * @brief Handles the type builtin command.
 *        Reports whether a command is a shell builtin or an executable in PATH.
 * @param pathbuf (char *) Buffer used to store a found executable path.
 * @param pathbuf_size (size_t) Size of the path buffer.
 * @param argc (int) Number of command line arguments.
 * @param argv (char *[]) Argument list passed to type.
 */
void builtin_type(char *pathbuf, size_t pathbuf_size, int argc, char *argv[]) {
  if (argc < 2) return;

  char *arg = argv[1];
  if (is_builtin(arg))
    printf("%s is a shell builtin\n", arg);
  else if (parse_path(pathbuf, pathbuf_size, arg))
    printf("%s is %s\n", arg, pathbuf);
  else
    printf("%s: not found\n", arg);
}

/**
 * @brief Handles the pwd builtin command.
 *        Prints the current working directory.
 */
void builtin_pwd(void) {
  char cwd[COMMAND_SIZE];

  getcwd(cwd, sizeof(cwd)); // <unistd.h>
  printf("%s\n", cwd);
}

/**
 * @brief Handles the cd builtin command.
 *        Changes the current working directory, including basic ~ expansion.
 * @param pathbuf (char *) Buffer used when expanding paths like ~/...
 * @param pathbuf_size (size_t) Size of the path buffer.
 * @param arg (char *) Directory path argument passed to cd.
 * @return 1 if the directory changes successfully, 0 otherwise.
 */
int builtin_cd(char *pathbuf, size_t pathbuf_size, char *arg) {
  const char *home = getenv("HOME");
  if (!home) return 0;

  // Handle home directory
  if (arg == NULL || arg[0] == '\0')
    arg = (char*)home;
  else if (arg[0] == '~') {
    if (arg[1] == '\0')
      arg = (char*)home;
    else if (arg[1] == '/') { // path of the form ~/...
      snprintf(pathbuf, pathbuf_size, "%s%s", home, arg + 1);
      arg = pathbuf;
    }
  }

  if (chdir(arg) != 0)
    return 0;
  return 1;
}

int run_builtin(char *pathbuf, size_t pathbuf_size, int argc, char *argv[]) {
  if (strcmp(argv[0], "echo") == 0) {
    builtin_echo(argc, argv);
    return 1;
  }
  else if (strcmp(argv[0], "type") == 0) {
    builtin_type(pathbuf, pathbuf_size, argc, argv);
    return 1;
  }
  else if (strcmp(argv[0], "pwd") == 0) {
    builtin_pwd();
    return 1;
  }
  else if (strcmp(argv[0], "cd") == 0) {
    if (builtin_cd(pathbuf, pathbuf_size, argv[1]) == 0)
      printf("%s: %s: No such file or directory\n", argv[0], argv[1]);
    return 1;
  }

  return 0;
}

/**
 * @brief Inserts all shell builtin command names into a Trie.
 * @param root (Trie *) Trie object used for builtin autocompletion.
 */
void build_builtin_trie(Trie *root) {
    for (size_t i = 0; i < sizeof(builtins)/sizeof(builtins[0]); i++) {
        trie_insert(root, builtins[i]);
    }
}
