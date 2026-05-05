#include "shell.h"
#include "builtins.h"
#include "redirection.h"
#include "executor.h"
#include "tokenizer.h"
#include "trie.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/**
 * @brief   An enum object to implement shell status codes.
 */
typedef enum {
    SHELL_CONTINUE,     /* Indicates shell is good to continue in execution */
    SHELL_EXIT          /* Shell exit code */
} ShellStatus;

void run_shell(Trie *builtin_trie);
static ShellStatus execute_command(int argc, char *argv[], Redirection *redir);
static int read_command(char *command, size_t command_size, Trie *builtin_trie);
static int read_command_interactive(char *command, size_t command_size, Trie *builtin_trie);
static void handle_tab_completion(char *command, size_t *length, size_t command_size,
                                  Trie *builtin_trie);
static int get_builtin_prefix(char *command, size_t length, char **prefix);

/**
 * @brief Runs the main shell loop.
 *        Reads commands, tokenizes input, and executes each command.
 */
void run_shell(Trie *builtin_trie) {
  char command[COMMAND_SIZE];

  while (1) {
    if (!read_command(command, sizeof(command), builtin_trie))
      break;

    char *args[MAX_ARGS];
    int arg_count = tokenize_arg(command, args, MAX_ARGS);
    if (arg_count == 0)
      continue;

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
 * @param builtin_trie (Trie *) Trie used to autocomplete builtin command names.
 */
static int read_command(char *command, size_t command_size, Trie *builtin_trie) {
    printf("$ ");
    if (isatty(STDIN_FILENO))
        return read_command_interactive(command, command_size, builtin_trie);

    if (!fgets(command, command_size, stdin))
        return 0;
    command[strcspn(command, "\r\n")] = '\0'; // Strip off newline

    return 1;
}

/**
 * @brief Reads command input from an interactive terminal.
 *        Uses raw terminal mode to handle Enter, Tab, Backspace, and Ctrl-D.
 *        Returns 1 if a command is read, 0 on EOF or input failure.
 * @param command (char *) Buffer used to store the command line.
 * @param command_size (size_t) Size of the command buffer.
 * @param builtin_trie (Trie *) Trie used to autocomplete builtin command names.
 */
static int read_command_interactive(char *command, size_t command_size, Trie *builtin_trie) {
    struct termios original;
    struct termios raw;
    size_t length = 0;

    if (tcgetattr(STDIN_FILENO, &original) == -1)
        return 0;

    raw = original;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1)
        return 0;

    command[0] = '\0';
    while (1) {
        char c;
        ssize_t bytes_read = read(STDIN_FILENO, &c, 1);

        if (bytes_read <= 0) {
            tcsetattr(STDIN_FILENO, TCSANOW, &original);
            return 0;
        }

        if (c == '\n' || c == '\r') {
            command[length] = '\0';
            printf("\n");
            tcsetattr(STDIN_FILENO, TCSANOW, &original);
            return 1;
        }

        if (c == '\t') {
            handle_tab_completion(command, &length, command_size, builtin_trie);
            continue;
        }

        /* ASCII code 4 = EOT */
        if (c == 4 && length == 0) {
            tcsetattr(STDIN_FILENO, TCSANOW, &original);
            return 0;
        }

        /* ASCII code 127 = DEL */
        if ((c == 127 || c == '\b') && length > 0) {
            length--;
            command[length] = '\0';
            printf("\b \b");
            continue;
        }

        if (isprint((unsigned char)c) && length + 1 < command_size) {
            command[length++] = c;
            command[length] = '\0';
            printf("%c", c);
        }
    }
}

/**
 * @brief Completes the current builtin command prefix when Tab is pressed.
 *        Adds the missing characters and a trailing space when there is exactly one match.
 * @param command (char *) Current command input buffer.
 * @param length (size_t *) Current length of the command input buffer.
 * @param command_size (size_t) Size of the command buffer.
 * @param builtin_trie (Trie *) Trie used to find builtin command completions.
 */
static void handle_tab_completion(char *command, size_t *length, size_t command_size,
                                  Trie *builtin_trie) {
    char *prefix;
    char match[COMMAND_SIZE];

    if (!get_builtin_prefix(command, *length, &prefix)) {
        printf("\a");
        return;
    }

    int matches = trie_complete_unique(builtin_trie, prefix, match, sizeof(match));
    if (matches != 1) {
        printf("\a");
        return;
    }

    size_t prefix_offset = (size_t)(prefix - command);
    size_t prefix_length = *length - prefix_offset;
    size_t match_length = strlen(match);

    for (size_t i = prefix_length; i < match_length && *length + 1 < command_size; i++) {
        command[*length] = match[i];
        (*length)++;
        command[*length] = '\0';
        printf("%c", match[i]);
    }

    if (*length + 1 < command_size) {
        command[*length] = ' ';
        (*length)++;
        command[*length] = '\0';
        printf(" ");
    }
}

/**
 * @brief Finds the command-name prefix that should be autocompleted.
 *        Returns 1 only when the input contains a single unfinished command word.
 * @param command (char *) Current command input buffer.
 * @param length (size_t) Current length of the command input buffer.
 * @param prefix (char **) Stores a pointer to the prefix inside command.
 */
static int get_builtin_prefix(char *command, size_t length, char **prefix) {
    size_t i = 0;

    while (i < length && isspace((unsigned char)command[i]))
        i++;
    if (i == length)
        return 0;

    *prefix = &command[i];
    for (; i < length; i++) {
        if (isspace((unsigned char)command[i]))
            return 0;
    }

    return 1;
}
