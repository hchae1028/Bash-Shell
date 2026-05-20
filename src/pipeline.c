#include "pipeline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <spawn.h>

extern char **environ;

static int process_pipelined_command(pid_t *pid, int target_fd, int pipefd[2], char *argv[]);
static int split_pipeline(char *argv[], char ***left_argv, char ***right_argv);

void execute_pipeline(char *argv[]) {
    char **left_argv = NULL;
    char **right_argv = NULL;
    int pipefd[2];
    pid_t left_pid, right_pid;
    int status;

    if (!split_pipeline(argv, &left_argv, &right_argv))
        return;
    if (pipe(pipefd) == -1)
        return;
    
    if (process_pipelined_command(&left_pid, STDOUT_FILENO, pipefd, left_argv)) {
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }
    if (process_pipelined_command(&right_pid, STDIN_FILENO, pipefd, right_argv)) {
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

static int process_pipelined_command(pid_t *pid, int target_fd, int pipefd[2], char *argv[]) {
    posix_spawn_file_actions_t actions;
    int rc;

    rc = posix_spawn_file_actions_init(&actions);
    if (rc != 0)
        return rc;

    if (target_fd == STDOUT_FILENO) {
        rc = posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO);
        if (rc != 0)
            goto cleanup;
    }
    else if (target_fd == STDIN_FILENO) {
        rc = posix_spawn_file_actions_adddup2(&actions, pipefd[0], STDIN_FILENO);
        if (rc != 0)
            goto cleanup;
    }
    
    rc = posix_spawn_file_actions_addclose(&actions, pipefd[0]);
    if (rc != 0)
        goto cleanup;
    
    rc = posix_spawn_file_actions_addclose(&actions, pipefd[1]);
    if (rc != 0)
        goto cleanup;
    
    rc = posix_spawnp(pid, argv[0], &actions, NULL, argv, environ);
    if (rc != 0)
        goto cleanup;

    posix_spawn_file_actions_destroy(&actions);
    return 0;

cleanup:
    posix_spawn_file_actions_destroy(&actions);
    return rc;
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
