
## lte text editor

**L**uddite **T**ext **E**ditor  

lte is made for my personal use, it is not ment to cater to a general audience
lte is inspired by vi and kakoune
lte is usable, but unfinished

### keybinds

```
+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+--------------+
|         |         |         |         |         |*swap a c|         |         |         |         |         |         |         |              |
|         |         |         |         |         |         |         |         |         |         |         |         |         |              |
|         |    1    |    2    |    3    |    4    |    5    |    6    |    7    |    8    |    9    |    0    |         |         |              |
+---------+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+---------+
|              |     quit|         |1        |2        |         |2        |     redo|   anchor|    above|   anchor|         |         |         |
|              |         |         |word     |replace  |         |yank     |         |edit     |open     |paste    |         |         |         |
|              |   w quit|    write|         |         |         |         |     undo|   cursor|    below|   cursor|         |         |         |
+--------------+--+------+--+------+--+------+--+------+--+------+--+------+--+------+--+------+--+------+--+------+--+------+--+------+---------+
|                 |         |*     all|2        |         |1        |1        |1        |1        |1        |   record|         |                |
|                 |         |replace s|delete   |         |goto     |left     |down     |up       |right    |macro*   |         |                |
|                 |         |    first|         |         |         |         |         |         |         |  execute|         |                |
+----------------------+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----------------+
|                      |1        |1        |2        |         |1        |1        |1        |1*       |1*       |*    prev|                     |
|                      |line star|line end |change   |         |word back|para down|para up  |find prev|find next|find sub |                     |
|                      |         |         |         |         |         |         |         |         |         |     next|                     |
+----------------------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------+---------------------+

[1] = default changes selection; SHIFT extends selection
[2] = default effects selection; SHIFT effects line
[*] = not yet implmented

select all
```

## todo  

macro/repeat support

open non-existant file

select all

find in selection command

subsitute in selection command

refactor DrawScreen() to cashe lines

lte -h

## bugs

something with redo and paste

goto overshoots by one

everyting gets fucked for line longer then screen

tabs

resize terminal
