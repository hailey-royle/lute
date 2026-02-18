
## lte text editor

**L**uddite **T**ext **E**ditor

lte is made for my personal use, it is not ment to cater to a general audience
lte is inspired by vi and kakoune
lte is usable, but unfinished

### keybinds

```
+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+--------------+
|         |         |         |         |         |*swap a c|         |         |* sel all|         |         |         |         |              |
|         |         |         |         |         |         |         |         |         |         |         |         |         |              |
|         |    1    |    2    |    3    |    4    |    5    |    6    |    7    |    8    |    9    |    0    |         |         |              |
+---------+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+---------+
|              |     quit|         |1        |2        |         |2        |     redo|   anchor|    above|   anchor|         |         |         |
|              |         |         |word     |replace  |         |yank     |         |edit     |open     |paste    |         |         |         |
|              |   w quit|    write|         |         |         |         |     undo|   cursor|    below|   cursor|         |         |         |
+--------------+--+------+--+------+--+------+--+------+--+------+--+------+--+------+--+------+--+------+--+------+--+------+--+------+---------+
|                 |         |      all|2        |         |1        |1        |1        |1        |1        |   record|         |                |
|                 |         |f/r str* |delete   |         |goto     |left     |down     |up       |right    |macro*   |         |                |
|                 |         |    first|         |         |         |         |         |         |         |  execute|         |                |
+----------------------+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----------------+
|                      |1        |1        |2        |         |1        |1        |1        |1        |1        |         |                     |
|                      |line star|line end |change   |         |word back|para down|para up  |find prev|find next|         |                     |
|                      |         |         |         |         |         |         |         |         |         |         |                     |
+----------------------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------------------+

[1] = default changes selection; SHIFT extends selection
[2] = default effects selection; SHIFT effects line
[*] = not yet implmented
```

### todo

- macro/repeat support
- open non-existant file
- swap anchor and cursor
- select all
- find/replace in selection command
- lte -h
- resize terminal

### bugs

- something with redo and paste
- everyting gets fucked for line longer then screen
- tabs
