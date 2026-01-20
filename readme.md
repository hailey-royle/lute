# lte text editor

**L**uddite **T**ext **E**ditor  

lte is a vi like text editor made for personal use  
lte is usable, but unfinished

commands are in three parts;  
[command] [count] [motion]  
command defualts to [move]  
count defaults to [1]  
motion defaults to [none]  

*currently, only the move command is implemented*

Some actions fall outside of this system, such as quit and write

## keybindings

- q - quit
- w - write
- i - [mode] = edit // ignores other parameters
- a - [mode] = edit, [motion] = char right // ignores other parameters
- d - [command] = delete
- c - [command] = change
- h - [motion] = char left
- l - [motion] = char right
- b - [motion] = word left
- e - [motion] = word right
- k - [motion] = line up
- j - [motion] = line down
- n - [motion] = paragraph up
- m - [motion] = papargaph down
- z - [motion] = line start
- x - [motion] = line end
- g - [motion] = goto line [count]

## planned keybindings

- u - undo
- r - redo
- ? - repeat last command
- f - find "string"
- s - subsitute "string" for "string"
- o - open
- y - yeet
- p - paste
