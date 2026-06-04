# GNU Bash Shell

This is my attempt to recreate the Bash shell in C, according to the Bash Reference Manual v5.3.
The objective of the whole project is to deepen my understanding of computer systems.

To build and run, do `make && ./shell`.

## Supporting Features

Currently, the program supports the following features:
* I/O redirection
* Handling single/double quotes
* Tab autocompletion for builtin commands and executables in $PATH
* Dual pipeline

All builtins are stored in `src/builtins.c`, but only the follwing work:
* `echo`
* `type`
* `pwd`
* `cd`
* `exit`

## Example I/O

### Multiple matches
```
$ ls   
ls  lsappinfo  lsbom lskq  lsm  lsmp  lsof  lsvfs
$ ls
```
### Pipeline
```
$ echo hello world | wc -c
      12
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

## References

Below are the references I found helpful building the program:
* [Bash Reference Manual](https://www.gnu.org/software/bash/manual/bash.html)
* [The Open Group Base Specifications Issue 8 IEEE Std 1003.1™-2024 Edition](https://pubs.opengroup.org/onlinepubs/9799919799/)
* Advanced Programming in the UNIX Environment by W. Richard Stevens