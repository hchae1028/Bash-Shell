# GNU Bash Shell

This is my attempt to recreate the Bash shell in C, according to the Bash Reference Manual v5.3.
The objective of the whole project is to deepen my understanding of computer systems.

To build and run, do
`make && ./shell`

## Supporting Features

Currently, the program supports the following features:
* I/O redirection
* Handling single/double quotes
* Tab autocompletion for builtin commands and executables in $PATH
    (list all the matches in case of multiple matches)
* Dual pipeline

All builtins are stored in `src/builtins.c`, but only the follwing work:
* `echo`
* `type`
* `pwd`
* `cd`
* `exit`

## References

Below are the references I found helpful building the program:
* [Bash Reference Manual](https://www.gnu.org/software/bash/manual/bash.html)
* [The Open Group Base Specifications Issue 8 IEEE Std 1003.1™-2024 Edition](https://pubs.opengroup.org/onlinepubs/9799919799/)
* Advanced Programming in the UNIX Environment by W. Richard Stevens