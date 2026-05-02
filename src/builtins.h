#ifndef BUILTINS_H
#define BUILTINS_H

#include <stdlib.h>

/**
 * @brief Checks whether a given command argument is a shell builtin.
 *        Returns 1 if it is a shell builtin, 0 otherwise.
 * @param arg (const char *) Command line argument to be checked.
 */
int is_builtin(const char *command);

/**
 * @brief Handles the echo builtin command.
 *        Prints each argument after the command name, separated by spaces.
 * @param argc (int) Number of command line arguments.
 * @param argv (char *[]) Argument list passed to echo.
 */
void builtin_echo(int argc, char *argv[]);

/**
 * @brief Handles the type builtin command.
 *        Reports whether a command is a shell builtin or an executable in PATH.
 * @param pathbuf (char *) Buffer used to store a found executable path.
 * @param pathbuf_size (size_t) Size of the path buffer.
 * @param argc (int) Number of command line arguments.
 * @param argv (char *[]) Argument list passed to type.
 */
void builtin_type(char *pathbuf, size_t pathbuf_size, int argc, char *argv[]);

/**
 * @brief Handles the pwd builtin command.
 *        Prints the current working directory.
 */
void builtin_pwd(void);

/**
 * @brief Handles the cd builtin command.
 *        Changes the current working directory, including basic ~ expansion.
 *        Returns 1 if the directory changes successfully, 0 otherwise.
 * @param pathbuf (char *) Buffer used when expanding paths like ~/...
 * @param pathbuf_size (size_t) Size of the path buffer.
 * @param arg (char *) Directory path argument passed to cd.
 */
int builtin_cd(char *pathbuf, size_t pathbuf_size, char *arg);

#endif
