#include "shell.h"
#include "builtins.h"
#include "redirection.h"
#include "executor.h"
#include "tokenizer.h"
#include <stdio.h>
#include <string.h>

typedef enum {
  SHELL_CONTINUE,
  SHELL_EXIT
} ShellStatus;

void run_shell(void);
static ShellStatus execute_command(int argc, char *argv[], Redirection *redir);
static int read_command(char *command, size_t command_size);

/**
 * @brief Runs the main shell loop.
 *        Reads commands, tokenizes input, and executes each command.
 */
void run_shell(void) {
    char command[COMMAND_SIZE];

  while (1) {
    if (!read_command(command, sizeof(command))) break;

    char *args[MAX_ARGS];
    int arg_count = tokenize_arg(command, args, MAX_ARGS);
    if (arg_count == 0) continue;

    Redirection redir;
    arg_count = extract_redirs(args, &redir);
    if (arg_count == -1) {
      fprintf(stderr, "syntax error: expected filename after redirection\n");
      continue;
    }

    if (execute_command(arg_count, args, &redir) == SHELL_EXIT)
      break;
  }
}

/**
 * @brief Executes one parsed shell command.
 *        Dispatches shell builtins or runs an external program.
 *        Returns SHELL_EXIT when the shell should stop, SHELL_CONTINUE otherwise.
 * @param argc (int) Number of command line arguments.
 * @param argv (char *[]) Tokenized command argument list.
 * @param redir (Redirection *) Stores info about stdout/stderr redirection filenames and whether to append.
 */
static ShellStatus execute_command(int argc, char *argv[], Redirection *redir) {
    char pathbuf[COMMAND_SIZE];
    char *command = argv[0];
    int saved_stdout = -1;
    int saved_stderr = -1;

    if (command == NULL)
        return SHELL_CONTINUE;

    if (strcmp(command, "exit") == 0)
        return SHELL_EXIT;

    if (redir->out_file && is_builtin(command)) {
        saved_stdout = redirect_stdout(redir->out_file, redir->out_append);
        if (saved_stdout == -1) {
        perror(redir->out_file);
        return SHELL_CONTINUE;
        }
    }
    if (redir->err_file && is_builtin(command)) {
        saved_stderr = redirect_stderr(redir->err_file, redir->err_append);
        if (saved_stderr == -1) {
        if (saved_stdout != -1)
            restore_stdout(saved_stdout);
        perror(redir->err_file);
        return SHELL_CONTINUE;
        }
    }

    if (strcmp(command, "echo") == 0)
        builtin_echo(argc, argv);
    else if (strcmp(command, "type") == 0)
        builtin_type(pathbuf, sizeof(pathbuf), argc, argv);
    else if (strcmp(command, "pwd") == 0)
        builtin_pwd();
    else if (strcmp(command, "cd") == 0) {
        if (builtin_cd(pathbuf, sizeof(pathbuf), argv[1]) == 0)
        printf("%s: %s: No such file or directory\n", command, argv[1]);
    }
    else {
        if (run_program(argv, redir) != 0)
        printf("%s: command not found\n", command);
    }

    if (saved_stdout != -1)
        restore_stdout(saved_stdout);
    if (saved_stderr != -1)
        restore_stderr(saved_stderr);
    return SHELL_CONTINUE;
}

/**
 * @brief Reads one line of input from the user.
 *        Prints the prompt and removes the trailing newline.
 *        Returns 1 if a command is read, 0 on EOF or input failure.
 * @param command (char *) Buffer used to store the command line.
 * @param command_size (size_t) Size of the command buffer.
 */
static int read_command(char *command, size_t command_size) {
  printf("$ ");
  if (!fgets(command, command_size, stdin)) return 0;
  command[strcspn(command, "\r\n")] = '\0'; // Strip off newline

  return 1;
}