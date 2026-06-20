
## lute text editor

**Lu**ddite **T**ext **E**ditor

lute is made for my personal use, it is not ment to cater to a general audience.  
lute is inspired by vi and kakoune.  
lute is usable, but unfinished.  


### To Build

`$ make build`


### To Install

`# make build install`


### keybinds

```
+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-----------+
|       |       |       |       |       |       |       |       |       |( insid|) insid|       |       |           |
|       |       |       |       |       |       |       |       |       |       |       |       |       |           |
|       |   1   |   2   |   3   |   4   |   5   |   6   |   7   |   8   |   9   |   0   |       |       |           |
+-------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+-------+
|           |   quit|       |       |l repla|  f end| l yank|   redo|       |       |       |{ insid|} insid|       |
|           |       |       |       |       |       |       |       |       |       |       |       |       |       |
|           | w quit|  write| w next|replace|f start|   yank|   undo|   edit| l open|  paste|[ insid|] insid|       |
+-----------+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+-------+
|              |ac swap|l split|l delet|fc prev|f selec|       |       |       |       |s rotat|" insid|            |
|              |       |       |       |       |       |       |       |       |       |       |       |            |
|              |a u/pin|s split| delete|fc next|   goto| c prev| l next| l prev| c next|unsplit|' insid|            |
+--------------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+----------------+
|                  |       |       |l chang|       |       |       |       |uninden| indent|       |                |
|                  |       |       |       |       |       |       |       |       |       |       |                |
|                  |l start|  l end| chagne|       | w prev| p next| p prev|       |       |       |                |
+----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+--------+-------+
|          |       |       |       |                                       |       |       |       |        |       |
|          |       |       |       |                                       |       |       |       +--------+       |
|          |       |       |       |                                       |       |       |       |        |       |
+----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+--------+-------+

[*] = not yet implmented
```

### todo

- config.h
- horizontal scroll
- system clipboard
- repete command?
- open line above?
- key escape code cull?

### bugs

- redo/undo off by one (?) very rarely
- at least two asserts tripped when pasting in large amounts of data

