
## lute text editor

**L**uddite **T**ext **E**ditor

lute is made for my personal use, it is not ment to cater to a general audience
lute is inspired by vi and kakoune
lute is usable, but unfinished

### keybinds

```
+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-----------+
|       |       |       |       |       |       |       |swap ac|sel all|       |       |       |       |           |
|       |       |       |       |       |       |       |       |       |       |       |       |       |           |
|       |   1   |   2   |   3   |   4   |   5   |   6   |   7   |   8   |   9   |   0   |       |       |           |
+-------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+-------+
|           |   quit|       |1      |2      |       |2      |   redo| anchor|  above| anchor|       |       |       |
|           |       |       |w next |replace|       |yank   |       |edit   |open   |paste  |       |       |       |
|           | w quit|  write|       |       |       |       |   undo| cursor|  below| cursor|       |       |       |
+-----------+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+-------+
|              |       |    all|2      |       |1      |1      |1      |1      |1      | record|       |            |
|              |       |fr str*|delete |       |goto   |c prev |l down |l up   |c next |macro  |       |            |
|              |       |  first|       |       |       |       |       |       |       |execute|       |            |
+--------------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+----------------+
|                  |1      |1      |2      |       |1      |1      |1      |1      |1      |       |                |
|                  |l start|l end  |change |       |w back |p down |p up   |f prev |f next |       |                |
|                  |       |       |       |       |       |       |       |       |       |       |                |
+------------------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+----------------+

[1] = default changes selection; SHIFT extends selection
[2] = default effects selection; SHIFT effects line
[*] = not yet implmented
```

### todo

- find/replace in selection command
- tabs
- generic key prosessing
- figure out commandCount placmnet
- open non-existant file
- see foward in overextended line

### bugs

- highlight when cursor if off screen
