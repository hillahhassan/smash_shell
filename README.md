# Smash Shell

**Smash (Small Shell)** is a lightweight command-line shell implemented in C++ for Unix-like systems. It mimics the behavior of basic Bash functionality, supporting internal and external commands, job control, signals, redirection, and piping.

## Features

- Built-in commands: `cd`, `pwd`, `showpid`, `jobs`, `fg`, `quit`, `kill`, `chprompt`, `chmod`
- Timeout support via `timeout` command
- Background and foreground process handling
- Redirection (`>`, `>>`)
- Piping (`|`, `|&`)
- Signal handling for `Ctrl+C` (SIGINT) and `alarm` (SIGALRM)

## Build Instructions

Make sure you're on a Unix-like system with a C++ compiler installed (e.g. `g++`), then run:

```bash
make
