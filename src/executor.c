#include "executor.h"
#include <spawn.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

extern char **environ;

/**
 * @brief Runs an external program using posix_spawnp.
 *        Waits for the child process to finish before returning.
 * @param argv (char *[]) Argument list where argv[0] is the command name.
 * @param redir (Redirection *) Stores info about stdout/stderr redirection filenames and whether to append.
 * @return 0 if the program starts successfully, nonzero otherwise.
 */
int run_program(char *argv[], Redirection *redir) {
    pid_t pid;
    int status;
    int rc;
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_t *actions_ptr = NULL;

    if (redir->out_file || redir->err_file || redir->in_file) {
        rc = posix_spawn_file_actions_init(&actions);
        if (rc != 0)
            return rc;

        if (redir->out_file) {
            int out_flags = O_WRONLY | O_CREAT;
            if (redir->out_append)
                out_flags |= O_APPEND;
            else
                out_flags |= O_TRUNC;

            rc = posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, redir->out_file, out_flags, 0644);
            if (rc != 0) {
                posix_spawn_file_actions_destroy(&actions);
                return rc;
            }
        }

        if (redir->err_file) {
            int err_flags = O_WRONLY | O_CREAT;
            if (redir->err_append)
                err_flags |= O_APPEND;
            else
                err_flags |= O_TRUNC;

            rc = posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, redir->err_file, err_flags, 0644);
            if (rc != 0) {
                posix_spawn_file_actions_destroy(&actions);
                return rc;
            }
        }

        if (redir->in_file) {
            rc = posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, redir->in_file, O_RDONLY, 0);
            if (rc != 0) {
                posix_spawn_file_actions_destroy(&actions);
                return rc;
            }
        }

        actions_ptr = &actions;
    }

    rc = posix_spawnp(&pid, argv[0], actions_ptr, NULL, argv, environ);

    if (actions_ptr != NULL)
        posix_spawn_file_actions_destroy(&actions);

    if (rc == 0)
        waitpid(pid, &status, 0);
    return rc;
}
