
# lute text editor

**Lu**ddite **T**ext **E**ditor

lute is made for my personal use, it is not ment to cater to a general audience
lute is inspired by vi and kakoune
lute is usable, but unfinished

### keybinds

```
+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-----------+
|       |       |       |       |       |       |       |       |       |( insid|) insid|       |       |           |
|       |       |       |       |       |       |       |       |       |       |       |       |       |           |
|       |   1   |   2   |   3   |   4   |   5   |   6   |   7   |   8   |   9   |   0   |       |       |           |
+-------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+-------+
|           |   quit|       |       |l repla|  f end| l yank|   redo|       |la*open|       |{ insid|} insid|       |
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

- open line above?
- repete command
- key escape code cull
- utf8?
- horizontal scroll
- config.h

### bugs
- string.h:70: StringDelete: Assertion "string->len >= index + count" failed. String length is less than index + count
    mulit cursor de-indent, the lower of the two selections went to a previous line and did nothing, but the later of the two worked as intended.
    ? indent fucked up undo data, only got the assert on undo
