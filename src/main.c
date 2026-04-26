#include "tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <spawn.h>
#include <unistd.h>

extern char **environ;

#define COMMAND_SIZE 1024
#define MAX_ARGS 32

typedef enum {
  SHELL_CONTINUE,
  SHELL_EXIT
} ShellStatus;

static const char *builtins[] = {"echo", "type", "exit", "cd", "printf", "pwd",};

static int is_builtin(const char *arg) {
  for (size_t i = 0; i < sizeof(builtins)/sizeof(builtins[0]); i++) {
    if (strcmp(arg, builtins[i]) == 0)
      return 1;
  }
  return 0;
}

static int is_inpath(const char *path) {
  struct stat st;
  if (stat(path, &st) == 0) {
    // Permission check
    if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
      return 1;
  }
  return 0;
}

static int parse_path(char *pathbuf, size_t pathbuf_size, const char *arg) {
  const char *path = getenv("PATH");
  if (!path || !arg) return 0;

  char *path_copy = strdup(path);
  char *saveptr = NULL;
  char *dir = strtok_r(path_copy, ":", &saveptr);

  int found = 0;
  while (dir != NULL) {
    snprintf(pathbuf, pathbuf_size, "%s/%s", dir, arg);
    if (is_inpath(pathbuf) == 1) {
      found = 1;
      break;
    }
    dir = strtok_r(NULL, ":", &saveptr);
  }

  free(path_copy);
  return found;
}

static int run_program(char *argv[]) {
  pid_t pid;
  int status;
  int rc = posix_spawnp(&pid, argv[0], NULL, NULL, argv, environ);

  if (rc == 0)
    waitpid(pid, &status, 0);
  return rc;
}

static void builtin_echo(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    if (i > 1)
      printf(" ");
    printf("%s", argv[i]);
  }
  printf("\n");
}

static void builtin_type(char *pathbuf, size_t pathbuf_size, int argc, char *argv[]) {
  if (argc < 2) return;

  char *arg = argv[1];
  if (is_builtin(arg))
    printf("%s is a shell builtin\n", arg);
  else if (parse_path(pathbuf, pathbuf_size, arg))
    printf("%s is %s\n", arg, pathbuf);
  else
    printf("%s: not found\n", arg);
}

static void builtin_pwd(void) {
  char cwd[COMMAND_SIZE];

  getcwd(cwd, sizeof(cwd));
  printf("%s\n", cwd);
}

static int builtin_cd(char *pathbuf, size_t pathbuf_size, char *arg) {
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

static ShellStatus execute_command(int argc, char *argv[]) {
  char pathbuf[COMMAND_SIZE];
  char *command = argv[0];

  if (command == NULL)
    return SHELL_CONTINUE;

  if (strcmp(command, "exit") == 0)
    return SHELL_EXIT;

  if (strcmp(command, "echo") == 0)
    builtin_echo(argc, argv);
  else if (strcmp(command, "type") == 0)
    builtin_type(pathbuf, sizeof(pathbuf), argc, argv);
  else if (strcmp(command, "pwd") == 0)
    builtin_pwd();
  else if (strcmp(command, "cd") == 0) {
    if (builtin_cd(pathbuf, sizeof(pathbuf), argv[1]) == 0)
      printf("%s: %s: No such file or directory\n", command, argv[1]);
  }
  else {
    if (run_program(argv) != 0)
      printf("%s: command not found\n", command);
  }

  return SHELL_CONTINUE;
}

static int read_command(char *command, size_t command_size) {
  printf("$ ");
  if (!fgets(command, command_size, stdin)) return 0;
  command[strcspn(command, "\r\n")] = '\0'; // Strip off newline

  return 1;
}

static void run_shell(void) {
  char command[COMMAND_SIZE];

  while (1) {
    if (!read_command(command, sizeof(command))) break;

    char *args[MAX_ARGS];
    int arg_count = tokenize_arg(command, args, MAX_ARGS);
    if (arg_count == 0) continue;

    if (execute_command(arg_count, args) == SHELL_EXIT)
      break;
  }
}

int main(void) {
  // Flush after every printf
  setbuf(stdout, NULL);

  run_shell();

  return 0;
}
