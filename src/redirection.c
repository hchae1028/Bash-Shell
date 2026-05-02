#include "redirection.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int extract_redirs(char *argv[], Redirection *redir);
int redirect_stdout(const char *file, int append);
int redirect_stderr(const char *file, int append);
void restore_stdout(int saved_stdout);
void restore_stderr(int saved_stderr);
static int is_stdout_redir(const char *arg, int *append);
static int is_stderr_redir(const char *arg, int *append);

/**
 * @brief Extracts stdout redirection from an argument list.
 *        Removes the redirect operator and filename from argv.
 *        Returns the new argument count, or -1 if the filename is missing.
 * @param argv (char *[]) Argument list to scan and modify.
 * @param redir (Redirection *) Stores info about stdout/stderr redirection filenames and whether to append.
 */
int extract_redirs(char *argv[], Redirection *redir) {
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
        argv[argc++] = argv[i];
    }

    argv[argc] = NULL;
    return argc;
}

/**
 * @brief Redirects the shell process stdout to a file.
 *        Returns a saved copy of stdout, or -1 if redirection fails.
 * @param out_file (const char *) File to redirect stdout into.
 * @param append (int) Whether to append instead of truncating the file.
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
 *        Returns a saved copy of stderr, or -1 if redirection fails.
 * @param err_file (const char *) File to redirect stderr into.
 * @param append (int) Whether to append instead of truncating the file.
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
 * @brief Checks whether an argument is a stdout redirection operator.
 *        Supports >, 1>, >>, and 1>>.
 *        Returns 1 if it is a stdout redirect, 0 otherwise.
 * @param arg (const char *) Argument to check.
 * @param append (int *) Stores whether the redirect should append.
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
 *        Returns 1 if it is a stderr redirect, 0 otherwise.
 * @param arg (const char *) Argument to check.
 * @param append (int *) Stores whether the redirect should append.
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
