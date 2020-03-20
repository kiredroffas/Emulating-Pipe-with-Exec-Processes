  This C program is targeted at the Linux platform and performs like a shell pipeline.

  Example: ./connect ls : sort   on the command line would run equivalent to   ls | sort

  Program Interface:

    ./connect <arg1> : <arg2>        i.e.      ./connect ls
                                               ./connect ls -a -i
                                               ./connect ls -ai :  (will read in 2nd arg)
                                                                   (2nd arg no tags/spaces)
                                               ./connect ls -ia : sort
                                               ./connect ls -ia : sort -r

    Where: <arg1> and <arg2> are parameters that specify the programs to be run. If <arg1> 
    is specified without a colon or <arg2>, then <arg1> will be run as though there was not a 
    colon (pipe).

    If <arg1> : is entered it will act as <arg1> | would on the command line (read in arg2).
    The read in <arg2> cannot contain any spaces or flags.

  Within the program the colon breaks argv into a left (ls) and right (sort) portion. 
  Implementation then fork/exec and sets up a pipe such that:

  -Child process:  left portion, runs with stdout = pipe write end
  -Parent process: right portion, runs with stdin = pipe read end
  -Implementation could have swapped the parent/child processes, but putting the read end
   of the pipe in the parent process allows for the use of the wait() function to wait
   and ensure that the child process terminates first (child finishes writing before parent
   attempts to read).

  A Makefile is included with the following rules:

  -all: compiles everything together and produces an executable
  -clean: removes all object and temporary files
  -run: command for running the executable, must have single word commands (no tags/spaces)
        i.e.  make run arg1=ls arg2=sort   OR   make run arg1=ls       (will read in 2nd arg)
                                                make run arg1=ls arg2=

  If you want to run commands with multiple tags/spaces be sure to use command line executable
  i.e.  ./connect ls -i -a : sort -r    (argument tags won't work with make run command).

  Error catching is implemented to detect improper command line input as well as any system
  errors that may occur with the approriate errno and errno message. All errors are printed to
  stderr before exiting with non-zero exit code (1).
