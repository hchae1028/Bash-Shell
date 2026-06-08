#include "shell.h"
#include "builtins.h"
#include "redirection.h"
#include "executor.h"
#include "tokenizer.h"
#include "trie.h"
#include "pipeline.h"
#include "expansion.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
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

static ShellStatus execute_command(int argc, char *argv[]);
static int read_command(char *command, size_t command_size, Trie *builtin_trie, Trie *path_trie);
static int read_command_interactive(char *command, size_t command_size, Trie *builtin_trie, Trie *path_trie);
static void handle_tab_completion(char *command, size_t *length, size_t command_size,
                                  Trie *builtin_trie, Trie *path_trie);
static int get_builtin_prefix(char *command, size_t length, char **prefix);
static char *find_longest_common_prefix(CompletionResult *result);

/**
 * @brief Runs the main shell loop.
 *        Reads commands, tokenizes input, and executes each command.
 * @param builtin_trie (Trie *) Trie used to autocomplete builtin command names.
 * @param path_trie (Trie *) Trie used to autocomplete executable names from PATH.
 */
void run_shell(Trie *builtin_trie, Trie *path_trie) {
  char command[COMMAND_SIZE];
  char expanded[COMMAND_SIZE];  // Expanded buffer for environment variable expansion

  while (1) {
    if (!read_command(command, sizeof(command), builtin_trie, path_trie))
      break;
    
    if (!expand_env_var(command, expanded, sizeof(expanded))) {
        fprintf(stderr, "expansion error\n");
        continue;
    }

    char *args[MAX_ARGS];
    int arg_count = tokenize_arg(expanded, args, MAX_ARGS);
    if (arg_count == 0)
      continue;

    if (execute_command(arg_count, args) == SHELL_EXIT)
      break;
  }
}

/**
 * @brief Executes one parsed shell command.
 *        Dispatches shell builtins or runs an external program.
 *        Returns SHELL_EXIT when the shell should stop, SHELL_CONTINUE otherwise.
 * @param argc (int) Number of command line arguments.
 * @param argv (char *[]) Tokenized command argument list.
 */
static ShellStatus execute_command(int argc, char *argv[]) {
    char pathbuf[COMMAND_SIZE];
    char *command;
    int saved_stdin = -1;
    int saved_stdout = -1;
    int saved_stderr = -1;
    Redirection redir;

    if (argv[0] == NULL)
        return SHELL_CONTINUE;

    if (strcmp(argv[0], "exit") == 0)
        return SHELL_EXIT;

    if (find_pipeline_index(argv) != -1) {
        execute_pipeline(argv);
        return SHELL_CONTINUE;
    }

    argc = extract_redirs(argv, &redir);
    if (argc == -1) {
        fprintf(stderr, "syntax error: expected filename after redirection\n");
        return SHELL_CONTINUE;
    }
    if (argc == 0 || argv[0] == NULL)
        return SHELL_CONTINUE;

    command = argv[0];

    if (redir.out_file && is_builtin(command)) {
        saved_stdout = redirect_stdout(redir.out_file, redir.out_append);
        if (saved_stdout == -1) {
            perror(redir.out_file);
            return SHELL_CONTINUE;
        }
    }
    if (redir.err_file && is_builtin(command)) {
        saved_stderr = redirect_stderr(redir.err_file, redir.err_append);
        if (saved_stderr == -1) {
            if (saved_stdout != -1)
                restore_stdout(saved_stdout);
            perror(redir.err_file);
            return SHELL_CONTINUE;
        }
    }
    if (redir.in_file && is_builtin(command)) {
        saved_stdin = redirect_stdin(redir.in_file);
        if (saved_stdin == -1) {
            if (saved_stdout != -1)
                restore_stdout(saved_stdout);
            if (saved_stderr != -1)
                restore_stderr(saved_stderr);
            perror(redir.in_file);
            return SHELL_CONTINUE;
        }
    }

    if (run_builtin(pathbuf, sizeof(pathbuf), argc, argv)) {
        // Run builtin
    }
    else {
        if (redir.in_file && access(redir.in_file, R_OK) == -1) {
            perror(redir.in_file);
            return SHELL_CONTINUE;
        }
        if (run_program(argv, &redir) != 0)
        printf("%s: command not found\n", command);
    }

    if (saved_stdout != -1)
        restore_stdout(saved_stdout);
    if (saved_stderr != -1)
        restore_stderr(saved_stderr);
    if (saved_stdin != -1)
        restore_stdin(saved_stdin);
    return SHELL_CONTINUE;
}

/**
 * @brief Reads one line of input from the user.
 *        Prints the prompt and removes the trailing newline.
 *        Returns 1 if a command is read, 0 on EOF or input failure.
 * @param command (char *) Buffer used to store the command line.
 * @param command_size (size_t) Size of the command buffer.
 * @param builtin_trie (Trie *) Trie used to autocomplete builtin command names.
 * @param path_trie (Trie *) Trie used to autocomplete executable names from PATH.
 */
static int read_command(char *command, size_t command_size, Trie *builtin_trie, Trie *path_trie) {
    printf("$ ");
    if (isatty(STDIN_FILENO))
        return read_command_interactive(command, command_size, builtin_trie, path_trie);

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
 * @param path_trie (Trie *) Trie used to autocomplete executable names from PATH.
 */
static int read_command_interactive(char *command, size_t command_size, Trie *builtin_trie, Trie *path_trie) {
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
            handle_tab_completion(command, &length, command_size, builtin_trie, path_trie);
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
 * @brief Completes the current command prefix when Tab is pressed.
 *        Uses builtin and PATH executable matches. Completes a unique match,
 *        extends to the longest common prefix for partial matches, or lists
 *        possible matches when no more characters can be completed.
 * @param command (char *) Current command input buffer.
 * @param length (size_t *) Current length of the command input buffer.
 * @param command_size (size_t) Size of the command buffer.
 * @param builtin_trie (Trie *) Trie used to find builtin command completions.
 * @param path_trie (Trie *) Trie used to find executable command completions.
 */
static void handle_tab_completion(char *command, size_t *length, size_t command_size,
                                  Trie *builtin_trie, Trie *path_trie) {
    char *prefix;

    if (!get_builtin_prefix(command, *length, &prefix)) {
        printf("\a");
        return;
    }

    CompletionResult result;
    completion_result_init(&result);
    trie_add_matches(builtin_trie, prefix, &result);
    trie_add_matches(path_trie, prefix, &result);
    int matches = result.count;

    if (matches == 0) {
        printf("\a");
        return;
    }
    else if (matches > 1) {
        char *lcp = find_longest_common_prefix(&result);
        size_t prefix_offset = (size_t)(prefix - command);
        size_t prefix_length = *length - prefix_offset;
        size_t lcp_length = strlen(lcp);

        /* Multiple matches can still complete if they share more characters. */
        if (lcp_length > prefix_length) {
            for (size_t i = prefix_length; i < lcp_length && *length + 1 < command_size; i++) {
                command[*length] = lcp[i];
                (*length)++;
                command[*length] = '\0';
                printf("%c", lcp[i]);
            }
        }
        else {
            printf("\a\n");

            for (int i = 0; i < result.count; i++) {
                printf("%s", result.matches[i]);
                if (i + 1 < result.count)
                    printf("  ");
            }
            printf("\n$ %s", command);
        }

        free(lcp);
        return;
    }
    else {
        char *match = result.matches[0];
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

/**
 * @brief Finds the longest starting substring shared by all completion matches.
 *        Returns the longest common prefix string.
 * @param result (CompletionResult *) Completion matches to compare.
 */
static char *find_longest_common_prefix(CompletionResult *result) {
    char *common_prefix = strdup(result->matches[0]);

    for (size_t i = 0; i < result->count; i++) {
        size_t j = 0;

        while (result->matches[i][j] != '\0'
               && common_prefix[j] != '\0'
               && result->matches[i][j] == common_prefix[j]) {
            j++;
        }
        common_prefix[j] = '\0';
    }
    return common_prefix;
}
