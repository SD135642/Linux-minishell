# Linux-minishell

### Overview

This project implements a simple shell program that allows users to interact with their system through a command-line interface. The shell supports basic commands, including changing directories (cd), executing external programs, and exiting the shell (exit).

### Usage 

This program is designed to work in a Linux environment. To build, run the commands:

    cd <repository's directory> 
    make

To run this program, run the following command:

    ./minishell

You will see a command prompt indicating the current working directory:

    [/path/to/directory]$
    
Use this as you would use your regular command-line.

### Features

    Supports basic shell commands: cd, exit, and external programs.
    Provides a user-friendly command prompt displaying the current working directory.
    Handles signals, including SIGINT (Ctrl+C) for graceful termination.


### Notes

    Use the command "exit" or signals to exit the shell
