#include "pipeline.h"
#include "builtins.h"
#include "shell.h"
#include "redirection.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

static int process_pipelined_command(pid_t *pid, char *segment[], int prev, int pipefd[2], int has_next, Redirection *redir);
static int split_pipeline_seg(char *argv[], char **segments[]);
static int get_argc(char *argv[]);

void execute_pipeline(char *argv[]) {
    char **segments[MAX_PIPELINE];
    Redirection redirs[MAX_PIPELINE];
    pid_t pids[MAX_PIPELINE];
    int segment_count;
    int pid_count = 0;
    int status;
    int prev = -1;

    segment_count = split_pipeline_seg(argv, segments);
    if (segment_count == -1)
        return;

    for (int i = 0; i < segment_count; i++) {
        int argc = extract_redirs(segments[i], &redirs[i]);

        if (argc == -1) {
            fprintf(stderr, "syntax error: expected filename after redirection\n");
            return;
        }
        if (argc == 0 || segments[i][0] == NULL) {
            printf("parse error\n");
            return;
        }
        if (redirs[i].in_file && access(redirs[i].in_file, R_OK) == -1) {
            perror(redirs[i].in_file);
            return;
        }
    }

    for (int i = 0; i < segment_count; i++) {
        int pipefd[2] = {-1, -1};
        int has_next = (i < segment_count - 1);

        if (has_next && pipe(pipefd) == -1) {
            perror("pipe");
            if (prev != -1)
                close(prev);
            break;
        }

        if (process_pipelined_command(&pids[pid_count], segments[i], prev, pipefd, has_next,
                                      &redirs[i]) == -1) {
            perror("fork");
            if (prev != -1)
                close(prev);
            if (pipefd[0] != -1)
                close(pipefd[0]);
            if (pipefd[1] != -1)
                close(pipefd[1]);
            break;
        }
        pid_count++;

        if (prev != -1)
            close(prev);

        if (has_next) {
            close(pipefd[1]);
            prev = pipefd[0];
        }
        else {
            prev = -1;
        }
    }

    if (prev != -1)
        close(prev);

    for (int i = 0; i < pid_count; i++)
        waitpid(pids[i], &status, 0);
}

/**
 * @brief Checks whether a given list of arguments has a pipeline command "|".
 * @param argv (char *[]) Tokenized command argument list.
 * @return Index of the pipeline operator, or -1 if not found.
 */
int find_pipeline_index(char *argv[]) {
    for (int i = 0; argv[i] != NULL; i++) {
        if (strcmp(argv[i], "|") == 0)
            return i;
    }
    return -1;
}

static int process_pipelined_command(pid_t *pid, char *segment[], int prev, int pipefd[2], int has_next, Redirection *redir) {
    *pid = fork();

    if (*pid == -1)
        return -1;

    if (*pid == 0) {
        if (redir->in_file) {
            int fd = open(redir->in_file, O_RDONLY);
            if (fd == -1) {
                perror(redir->in_file);
                _exit(1);
            }
            if (dup2(fd, STDIN_FILENO) == -1) {
                close(fd);
                perror("dup2");
                _exit(1);
            }
            close(fd);
        }
        else if (prev != -1) {
            if (dup2(prev, STDIN_FILENO) == -1) {
                close(prev);
                perror("dup2");
                _exit(1);
            }
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
                close(fd);
                perror("dup2");
                _exit(1);
            }
            close(fd);
        }
        else if (has_next) {
            if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                perror("dup2");
                _exit(1);
            }
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
                close(fd);
                perror("dup2");
                _exit(1);
            }
            close(fd);
        }

        if (pipefd[0] != -1)
            close(pipefd[0]);
        if (pipefd[1] != -1)
            close(pipefd[1]);
        if (prev != -1)
            close(prev);

        if (is_builtin(segment[0])) {
            char pathbuf[COMMAND_SIZE];
            int argc = get_argc(segment);

            if (run_builtin(pathbuf, sizeof(pathbuf), argc, segment)) {
                fflush(stdout);
                fflush(stderr);
                _exit(0);
            }
        }
        execvp(segment[0], segment);
        perror(segment[0]);
        _exit(127); // error code 127: command not found
    }
    return 0;
}

static int split_pipeline_seg(char *argv[], char **segments[]) {
    int count = 1;
    segments[0] = argv;

    for (int i = 0; argv[i] != NULL; i++) {
        if (strcmp(argv[i], "|") == 0) {
            if (i == 0 || argv[i + 1] == NULL || strcmp(argv[i + 1], "|") == 0) {
                printf("parse error\n");
                return -1;
            }
            if (count >= MAX_PIPELINE) {
                printf("parse error: too many pipeline commands\n");
                return -1;
            }
            argv[i] = NULL;
            segments[count++] = &argv[i + 1];
        }
    }
    return count;
}

static int get_argc(char *argv[]) {
    int argc = 0;
    while (argv[argc] != NULL)
        argc++;

    return argc;
}
