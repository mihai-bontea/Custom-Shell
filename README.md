# Custom Shell


## Features

* bash-like prompt showing the user and current directory. (built with the help of the environmental values of **LOGNAME**, **SESSION_MANAGER** and **HOME**)

* Always starts in *home/user*, despite the actual location of the project.

* Supports the commands that the normal shell supports, and extra commands given as executables in the directory holding the project.

* Supports piping and redirection.

* Has command history

## Networking Part

* The server is multithreaded and accepts multiple connections at the same time


## Compilation Instructions

sudo apt-get install libreadline6-dev<br/>
*gcc -c -o UtilIO UtilIO.c*<br/>
*gcc -g -o main main.c -lreadline*<br/>
*gcc -g -o chmod chmod.c*<br/>
*gcc -g -o diff diff.c*<br/>
*gcc -g -o server server.c -pthread*<br/>
*gcc -g -o client client.c*<br/>

## Known Bugs

One bug is caused by the **readline** function, specifically that this command cannot recognise that it's not starting
at column 1 of the line, and as a consequence, for long enough commands it will start overwriting its own prompt. This goes
unnoticed when not using many pipes.

So when testing long chains of commands linked by pipes, an easy fix is to add a newline character after the prompt.
