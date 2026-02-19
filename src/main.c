#include "tokenizer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <spawn.h>
#include <unistd.h>

extern char **environ;
const char *builtins[] = {"echo", "type", "exit", "cd", "printf", "pwd",};

int is_builtin(char *arg) {
  for (int i = 0; i < sizeof(builtins)/sizeof(builtins[0]); i++) {
    if (strcmp(arg, builtins[i]) == 0)
      return 1;
  }
  return 0;
}

int is_inpath(char *path) {
  struct stat st;
  if (stat(path, &st) == 0) {
    // Permission check
    if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
      return 1;
  }
  return 0;
}

int parse_path(char *pathbuf, char *arg) {
  const char *path = getenv("PATH");
  if (!path || !arg) return 0;

  char *path_copy = strdup(path);
  char *saveptr = NULL;
  char *dir = strtok_r(path_copy, ":", &saveptr);

  int found = 0;
  while (dir != NULL) {
    snprintf(pathbuf, 1024, "%s/%s", dir, arg);
    if (is_inpath(pathbuf) == 1) {
      found = 1;
      break;
    }
    dir = strtok_r(NULL, ":", &saveptr);
  }

  free(path_copy);
  return found;
}



int run_program(char *argv[]) {
  pid_t pid;
  int status;
  int rc = posix_spawnp(&pid, argv[0], NULL, NULL, argv, environ);

  if (rc == 0)
    waitpid(pid, &status, 0);
  return rc;
}

int builtin_cd(char *pathbuf, char *arg) {
  const char *home = getenv("HOME");
  if (!home) return 0;

  // Handle home directory
  if (arg == NULL || arg[0] == '\0')
    arg = (char*)home;
  else if (arg[0] == '~') {
    if (arg[1] == '\0')
      arg = (char*)home;
    else if (arg[1] == '/') { // path of the form ~/...
      snprintf(pathbuf, 1024, "%s%s", home, arg + 1);
      arg = pathbuf;
    }
  }

  if (chdir(arg) != 0)
    return 0;
  return 1;
}

int main(int argc, char *argv[]) {
  char command[1024];
  char pathbuf[1024];
  char cwd[1024];
  // Flush after every printf
  setbuf(stdout, NULL);

  while (1) {
    printf("$ ");
    if(!fgets(command, sizeof(command), stdin)) break;
    command[strcspn(command, "\r\n")] = '\0'; // Strip off newline

    char *args[32];
    int n = tokenize_arg(command, args, 32);
    if (n == 0) continue;

    char *builtin = args[0];
    
    // Empty command
    if (builtin == NULL) 
      continue;

    if (strcmp(builtin, "exit") == 0) 
      break;
    else if (strcmp(builtin, "echo") == 0) {
      for (int i = 1; i < n; i++) {
        if (i > 1) 
          printf(" ");
        printf("%s", args[i]);
      }
      printf("\n");
    }
    else if (strcmp(builtin, "type") == 0) {
      if (n < 2) continue;

      char *arg = args[1];
      if (is_builtin(arg))
        printf("%s is a shell builtin\n", arg);
      else if (parse_path(pathbuf, arg))
        printf("%s is %s\n", arg, pathbuf);
      else 
        printf("%s: not found\n", arg);
    }
    else if (strcmp(builtin, "pwd") == 0) {
      getcwd(cwd, sizeof(cwd));
      printf("%s\n", cwd);
    }
    else if (strcmp(builtin, "cd") == 0) {
      char *arg = args[1];
      
      if (builtin_cd(pathbuf, arg) == 0)
        printf("%s: %s: No such file or directory\n", builtin, arg);
    }
    else {
      if (run_program(args) != 0)
        printf("%s: command not found\n", builtin);
    }
  }

  return 0;
}
