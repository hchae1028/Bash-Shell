#ifndef SHELL_H
#define SHELL_H

#define COMMAND_SIZE 1024
#define MAX_ARGS 32

/**
 * @brief Runs the main shell loop.
 *        Reads commands, tokenizes input, and executes each command.
 */
void run_shell(void);

#endif
