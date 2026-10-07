
## Lute

**Lu**ddite **T**ext **E**ditor

*Lute is in early developement, expect bugs*

This file has information about using Lute.
For information about configuring Lute, see config.h.
For information about the source code, see lute.c.

### Compiling / Installing

Lute only supports linux, but it should be simple to port to another os.
If you are having issues compiling Lute on *any* os, please open an issue.
*Note I do not have any development experience in any other os*

To compile: `$ compiler lute.c -o lute -O3 -std=c89`

To install: `$ sudo cp lute /usr/local/bin/`

### keybind.c

To compile: `$ compiler keybind.c -o keybind`

Reads the user input per key and returns the corresponding string.

### Default Keymap

