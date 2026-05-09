
# lute text editor

**Lu**ddite **T**ext **E**ditor

lute is made for my personal use, it is not ment to cater to a general audience
lute is inspired by vi and kakoune
lute is usable, but unfinished
### keybinds

```
+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-----------+
|       |       |       |       |       |       |       |       |       |       |       |       |       |           |
|       |       |       |       |       |       |       |       |       |       |       |       |       |           |
|       |   1   |   2   |   3   |   4   |   5   |   6   |   7   |   8   |   9   |   0   |       |       |           |
+-------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+-------+
|           |   quit|       |       |l delet|l chang|l repla| l copy|       |  *redo|       |       |       |       |
|           |       |       |       |       |       |       |       |       |       |       |       |       |       |
|           | w quit|  write|       | delete| change|replace|   copy|       |  *undo|  paste|       |       |       |
+-----------+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+-------+
|              |   swap|c fprev| p prev| w prev|       |  f end| w next| p next|c fnext|       |       |            |
|              |anchor |       |       |       |       |       |       |       |       |       |       |            |
|              | un/pin|l start| l prev| c prev|*goto c|f start| c next| l next|  l end|       |       |            |
+--------------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+----------------+
|                  |       |       |       |       |       |       |       |       |       |       |                |
|                  |       |       |       |       |       |       |       |       |       |       |                |
|                  |       |       |       |       |       |       |       |       |       |       |                |
+----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+--------+-------+
|          |       |       |       |                                       |       |       |       |        |       |
|          |       |       |       |                                       |       |       |       +--------+       |
|          |       |       |       |                                   edit|       |       |       |        |       |
+----------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+--------+-------+
[*] = not yet implmented
```

### todo

- split cursor line
- split cursor word
- split cursor string
- undo/redo
- compounds
- - open line
- - select all
- - select inside ( { [ ' "
- - indent
- repete command
- command count
- goto line
- key escape code cull
- utf8?
