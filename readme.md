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
- i - insert mode
- a - append
- h - char left
- l - char right 
- b - word left 
- e - word right
- k - line up
- j - line down 
- n - paragraph up 
- m - paragraph down
- z - line start
- x - line end 
- g - goto line [count]

## planned keybindings

- u - undo
- r - redo
- ? - repeat last command
- f - find "string"
- s - subsitute "string" for "string"
- o - open
- d - delete
- c - change
- y - yeet
- p - paste
