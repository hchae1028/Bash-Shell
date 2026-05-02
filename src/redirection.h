#ifndef REDIRECTION_H
#define REDIRECTION_H

typedef struct {
  char *out_file;
  char *err_file;
  int out_append;
  int err_append;
} Redirection;

/**
 * @brief Extracts stdout redirection from an argument list.
 *        Removes the redirect operator and filename from argv.
 *        Returns the new argument count, or -1 if the filename is missing.
 * @param argv (char *[]) Argument list to scan and modify.
 * @param redir (Redirection *) Stores info about stdout/stderr redirection filenames and whether to append.
 */
int extract_redirs(char *argv[], Redirection *redir);

/**
 * @brief Redirects the shell process stdout to a file.
 *        Returns a saved copy of stdout, or -1 if redirection fails.
 * @param out_file (const char *) File to redirect stdout into.
 * @param append (int) Whether to append instead of truncating the file.
 */
int redirect_stdout(const char *file, int append);

/**
 * @brief Redirects the shell process stderr to a file.
 *        Returns a saved copy of stderr, or -1 if redirection fails.
 * @param err_file (const char *) File to redirect stderr into.
 * @param append (int) Whether to append instead of truncating the file.
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

#endif
