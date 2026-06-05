#include "pipeline.h"
#include "builtins.h"
#include "shell.h"
#include "redirection.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

static int process_pipelined_command(pid_t *pid, int target_fd, int pipefd[2], char *argv[], Redirection *redir);
static int split_pipeline(char *argv[], char ***left_argv, char ***right_argv);
static int get_argc(char *argv[]);

void execute_pipeline(char *argv[], Redirection *redir) {
    char **left_argv = NULL;
    char **right_argv = NULL;
    int pipefd[2];
    pid_t left_pid, right_pid;
    int status;

    if (redir->in_file && access(redir->in_file, R_OK) == -1) {
        perror(redir->in_file);
        return;
    }

    if (!split_pipeline(argv, &left_argv, &right_argv))
        return;
    if (pipe(pipefd) == -1)
        return;

    if (process_pipelined_command(&left_pid, STDOUT_FILENO, pipefd, left_argv, redir)) {
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }
    if (process_pipelined_command(&right_pid, STDIN_FILENO, pipefd, right_argv, redir)) {
        close(pipefd[0]);
        close(pipefd[1]);
        waitpid(left_pid, &status, 0);
        return;
    }

    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(left_pid, &status, 0);
    waitpid(right_pid, &status, 0);
}

/**
 * @brief Checks whether a given list of arguments has a pipeline command "|".
 *        Returns the index of the pipeline command, -1 if not found.
 * @param argv (char *[]) Tokenized command argument list.
 */
int find_pipeline_index(char *argv[]) {
    for (int i = 0; argv[i] != NULL; i++) {
        if (strcmp(argv[i], "|") == 0)
            return i;
    }
    return -1;
}

static int process_pipelined_command(pid_t *pid, int target_fd, int pipefd[2], char *argv[], Redirection *redir) {
    *pid = fork();

    if (*pid == -1)
        return -1;

    if (*pid == 0) {
        if (target_fd == STDOUT_FILENO) {
            if (redir->in_file) {
                int fd = open(redir->in_file, O_RDONLY);
                if (fd == -1) {
                    perror(redir->in_file);
                    _exit(1);
                }

                if (dup2(fd, STDIN_FILENO) == -1) {
                    perror("dup2");
                    _exit(1);
                }
                close(fd);
            }

            if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                perror("dup2");
                _exit(1);
            }
        }
        else if (target_fd == STDIN_FILENO) {
            if (dup2(pipefd[0], STDIN_FILENO) == -1) {
                perror("dup2");
                _exit(1);
            }

            if (redir->out_file) {
                int flags = O_CREAT | O_WRONLY;
                if (redir->out_append)
                    flags |= O_APPEND;
                else
                    flags |= O_TRUNC;

                int fd = open(redir->out_file, flags, 0644);
                if (fd == -1) {
                    perror(redir->out_file);
                    _exit(1);
                }
                if (dup2(fd, STDOUT_FILENO) == -1) {
                    perror("dup2");
                    _exit(1);
                }
                close(fd);
            }

            if (redir->err_file) {
                int flags = O_CREAT | O_WRONLY;
                if (redir->err_append)
                    flags |= O_APPEND;
                else
                    flags |= O_TRUNC;

                int fd = open(redir->err_file, flags, 0644);
                if (fd == -1) {
                    perror(redir->err_file);
                    _exit(1);
                }
                if (dup2(fd, STDERR_FILENO) == -1) {
                    perror("dup2");
                    _exit(1);
                }
                close(fd);
            }
        }

        close(pipefd[0]);
        close(pipefd[1]);

        if (is_builtin(argv[0])) {
            char pathbuf[COMMAND_SIZE];
            int argc = get_argc(argv);

            if (run_builtin(pathbuf, sizeof(pathbuf), argc, argv)) {
                fflush(stdout);
                fflush(stderr);
                _exit(0);
            }
        }
        execvp(argv[0], argv);
        perror(argv[0]);
        _exit(127); // error code 127: command not found
    }
    return 0;
}

static int split_pipeline(char *argv[], char ***left_argv, char ***right_argv) {
    int pipe_index = find_pipeline_index(argv);

    if (pipe_index == -1)
        return 0;
    /* Missing left command; invalid */
    if (pipe_index == 0) {
        printf("parse error\n");
        return 0;
    }
    /* Missing right command; invalid */
    if (argv[pipe_index + 1] == NULL) {
        printf("parse error\n");
        return 0;
    }

    argv[pipe_index] = NULL;
    *left_argv = argv;
    *right_argv = &argv[pipe_index + 1];
    return 1;
}

static int get_argc(char *argv[]) {
    int argc = 0;
    while (argv[argc] != NULL)
        argc++;

    return argc;
}
