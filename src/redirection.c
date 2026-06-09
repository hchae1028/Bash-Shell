#include "redirection.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

static int is_stdout_redir(const char *arg, int *append);
static int is_stderr_redir(const char *arg, int *append);
static int is_stdin_redir(const char *arg);

/**
 * @brief Extracts redirection operators from an argument list.
 *        Removes the redirect operator and filename from argv.
 * @param argv (char *[]) Argument list to scan and modify.
 * @param redir (Redirection *) Stores info about stdout/stderr redirection filenames and whether to append.
 * @return New argument count, or -1 if a redirection filename is missing.
 */
int extract_redirs(char *argv[], Redirection *redir) {
    redir->in_file = NULL;
    redir->out_file = NULL;
    redir->err_file = NULL;
    redir->out_append = 0;
    redir->err_append = 0;

    int argc = 0;
    for (int i = 0; argv[i] != NULL; i++) {
        int redir_append;

        if (is_stdout_redir(argv[i], &redir_append)) {
          if (argv[i + 1] == NULL)
              return -1;

          redir->out_file = argv[i + 1];
          redir->out_append = redir_append;
          i++;
          continue;
        }

        if (is_stderr_redir(argv[i], &redir_append)) {
          if (argv[i + 1] == NULL)
              return -1;

          redir->err_file = argv[i + 1];
          redir->err_append = redir_append;
          i++;
          continue;
        }

        if (is_stdin_redir(argv[i])) {
          if (argv[i + 1] == NULL)
            return -1;

          redir->in_file = argv[i + 1];
          i++;
          continue;
        }

        argv[argc++] = argv[i];
    }

    argv[argc] = NULL;
    return argc;
}

/**
 * @brief Redirects the shell process stdout to a file.
 * @param out_file (const char *) File to redirect stdout into.
 * @param append (int) Whether to append instead of truncating the file.
 * @return Saved stdout file descriptor, or -1 if redirection fails.
 */
int redirect_stdout(const char *out_file, int append) {
  int flags = O_WRONLY | O_CREAT;
  int saved_stdout = dup(STDOUT_FILENO);
  if (saved_stdout == -1)
    return -1;

  if (append)
    flags |= O_APPEND;
  else
    flags |= O_TRUNC;

  int fd = open(out_file, flags, 0644);
  if (fd == -1) {
    close(saved_stdout);
    return -1;
  }

  if (dup2(fd, STDOUT_FILENO) == -1) {
    close(fd);
    close(saved_stdout);
    return -1;
  }

  close(fd);
  return saved_stdout;
}

/**
 * @brief Redirects the shell process stderr to a file.
 * @param err_file (const char *) File to redirect stderr into.
 * @param append (int) Whether to append instead of truncating the file.
 * @return Saved stderr file descriptor, or -1 if redirection fails.
 */
int redirect_stderr(const char *err_file, int append) {
  int flags = O_WRONLY | O_CREAT;
  int saved_stderr = dup(STDERR_FILENO);
  if (saved_stderr == -1)
    return -1;

  if (append)
    flags |= O_APPEND;
  else
    flags |= O_TRUNC;

  int fd = open(err_file, flags, 0644);
  if (fd == -1) {
    close(saved_stderr);
    return -1;
  }

  if (dup2(fd, STDERR_FILENO) == -1) {
    close(fd);
    close(saved_stderr);
    return -1;
  }

  close(fd);
  return saved_stderr;
}

/**
 * @brief Redirects the shell process stdin to a file.
 * @param in_file (const char *) File to redirect stdin from.
 * @return Saved stdin file descriptor, or -1 if redirection fails.
 */
int redirect_stdin(const char *in_file) {
  int saved_stdin = dup(STDIN_FILENO);

  if (saved_stdin == -1)
    return -1;
  int fd = open(in_file, O_RDONLY);
  if (fd == -1) {
    close(saved_stdin);
    return -1;
  }

  if (dup2(fd, STDIN_FILENO) == -1) {
    close(fd);
    close(saved_stdin);
    return -1;
  }

  close(fd);
  return saved_stdin;
}

/**
 * @brief Restores stdout after temporary redirection.
 * @param saved_stdout (int) Saved stdout file descriptor from redirect_stdout.
 */
void restore_stdout(int saved_stdout) {
  fflush(stdout);
  dup2(saved_stdout, STDOUT_FILENO);
  close(saved_stdout);
}

/**
 * @brief Restores stderr after temporary redirection.
 * @param saved_stderr (int) Saved stderr file descriptor from redirect_stderr.
 */
void restore_stderr(int saved_stderr) {
  fflush(stderr);
  dup2(saved_stderr, STDERR_FILENO);
  close(saved_stderr);
}

/**
 * @brief Restores stdin after temporary redirection.
 * @param saved_stdin (int) Saved stdin file descriptor from redirect_stdin.
 */
void restore_stdin(int saved_stdin) {
  dup2(saved_stdin, STDIN_FILENO);
  close(saved_stdin);
}

/**
 * @brief Checks whether an argument is a stdout redirection operator.
 *        Supports >, 1>, >>, and 1>>.
 * @param arg (const char *) Argument to check.
 * @param append (int *) Stores whether the redirect should append.
 * @return 1 if arg is a stdout redirection operator, 0 otherwise.
 */
static int is_stdout_redir(const char *arg, int *append) {
  if (strcmp(arg, ">") == 0 || strcmp(arg, "1>") == 0) {
    *append = 0;
    return 1;
  }
  if (strcmp(arg, ">>") == 0 || strcmp(arg, "1>>") == 0) {
    *append = 1;
    return 1;
  }
  return 0;
}

/**
 * @brief Checks whether an argument is a stderr redirection operator.
 *        Supports 2> and 2>>.
 * @param arg (const char *) Argument to check.
 * @param append (int *) Stores whether the redirect should append.
 * @return 1 if arg is a stderr redirection operator, 0 otherwise.
 */
static int is_stderr_redir(const char *arg, int *append) {
  if (strcmp(arg, "2>") == 0) {
    *append = 0;
    return 1;
  }
  if (strcmp(arg, "2>>") == 0) {
    *append = 1;
    return 1;
  }
  return 0;
}

/**
 * @brief Checks whether an argument is a stdin redirection operator, <.
 * @param arg (const char *) Argument to check.
 * @return 1 if arg is a stdin redirection operator, 0 otherwise.
 */
static int is_stdin_redir(const char *arg) {
  if (strcmp(arg, "<") == 0)
    return 1;
  return 0;
}
