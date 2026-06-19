# GNU Bash Shell

This is my attempt to recreate the Bash shell in C, according to the Bash Reference Manual v5.3.
The objective of the whole project is to deepen my understanding of computer systems.

To build and run, do `make && ./shell`.

## Supporting Features

Currently, the program supports the following features:
* I/O redirection
* Handling single/double quotes
* Tab autocompletion for builtin commands and executables in `$PATH`
* Multiple pipelines
* Environment variables expansion

All builtins are stored in `src/builtins.c`, but only the follwing work:
* `echo`
* `type`
* `pwd`
* `cd`
* `exit`

## Example I/O

### Multiple matches
```
$ pri<TAB>
printf  priclass.d  pridist.d  printenv  printf_gettext  printf_ngettext
$ printf<TAB>
printf  printf_gettext  printf_ngettext
$ printf
```
### Pipeline
```
$ echo hello world | wc -c
      12
$ type echo | cat | wc -c
      24
$ echo hello || wc -c
parse error
$ 
```
### Quote Handling
```
$ echo "hello \world"    "hi"
hello \world hi
$ echo 'helloworld\n'
helloworld\n
$ 
```
### Environment Variable Expansion
```
$ echo $PWD
/
$ cd bin
$ echo $PWD
/bin
$ 
```

## Limitations

* Most of the arrays used are static; a particular amount of memory is allocated by the numbers defined in header files.
Realistically, this should be more than enough space for processing most commands.
* Arrow keys to move the cursor in the input are not supported since the input is processed character by character via `termios.h`.
* More features such as history, job control are to be added.

## References

Below are the references I found helpful building the program:
* [Bash Reference Manual](https://www.gnu.org/software/bash/manual/bash.html)
* [The Open Group Base Specifications Issue 8 IEEE Std 1003.1™-2024 Edition](https://pubs.opengroup.org/onlinepubs/9799919799/)
* Advanced Programming in the UNIX Environment by W. Richard Stevens