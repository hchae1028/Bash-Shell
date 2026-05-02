#ifndef REDIRECTION_H
#define REDIRECTION_H

typedef struct {
  char *out_file;
  char *err_file;
  int out_append;
  int err_append;
} Redirection;

int extract_redirs(char *argv[], Redirection *redir);
int redirect_stdout(const char *file, int append);
int redirect_stderr(const char *file, int append);
void restore_stdout(int saved_stdout);
void restore_stderr(int saved_stderr);

#endif
