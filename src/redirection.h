#ifndef REDIRECTION_H
#define REDIRECTION_H

typedef struct {
  char *in_file;
  char *out_file;
  char *err_file;
  int out_append;
  int err_append;
} Redirection;

/**
 * @brief Extracts redirection operators from an argument list.
 *        Removes the redirect operator and filename from argv.
 * @param argv (char *[]) Argument list to scan and modify.
 * @param redir (Redirection *) Stores info about stdout/stderr redirection filenames and whether to append.
 * @return New argument count, or -1 if a redirection filename is missing.
 */
int extract_redirs(char *argv[], Redirection *redir);

/**
 * @brief Redirects the shell process stdout to a file.
 * @param out_file (const char *) File to redirect stdout into.
 * @param append (int) Whether to append instead of truncating the file.
 * @return Saved stdout file descriptor, or -1 if redirection fails.
 */
int redirect_stdout(const char *file, int append);

/**
 * @brief Redirects the shell process stderr to a file.
 * @param err_file (const char *) File to redirect stderr into.
 * @param append (int) Whether to append instead of truncating the file.
 * @return Saved stderr file descriptor, or -1 if redirection fails.
 */
int redirect_stderr(const char *file, int append);

/**
 * @brief Restores stdout after temporary redirection.
 * @param saved_stdout (int) Saved stdout file descriptor from redirect_stdout.
 */
void restore_stdout(int saved_stdout);

/**
 * @brief Restores stderr after temporary redirection.
 * @param saved_stderr (int) Saved stderr file descriptor from redirect_stderr.
 */
void restore_stderr(int saved_stderr);

/**
 * @brief Redirects the shell process stdin to a file.
 * @param in_file (const char *) File to redirect stdin from.
 * @return Saved stdin file descriptor, or -1 if redirection fails.
 */
int redirect_stdin(const char *in_file);

/**
 * @brief Restores stdin after temporary redirection.
 * @param saved_stdin (int) Saved stdin file descriptor from redirect_stdin.
 */
void restore_stdin(int saved_stdin);

#endif
