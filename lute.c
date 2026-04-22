#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "string.h"

#define START_ALT_SCREEN "\x1b[?1049h"
#define END_ALT_SCREEN "\x1b[?1049l"
#define CURSOR_HOME "\x1b[1;1H"
#define CURSOR_HIDDEN "\x1b[?25l"
#define CURSOR_SHOW "\x1b[?25h"
#define HIGHLIGHT_START "\x1b[41m"
#define HIGHLIGHT_END "\x1b[49m"

#define TAB_KEY 9
#define NEWLINE_KEY 10
#define LINEFEED_KEY 13
#define ESCAPE_KEY 27
#define SPACE_KEY 32
#define DELETE_KEY 127

struct edit {
        struct String insert;
        struct String delete;
        int cursor;
};

enum mode {
        COMMAND_MODE,
        EDIT_MODE,
};

enum record {
        RECORD_OFF,
        RECORD_ON,
};

struct lute {
        struct String file;
        struct String clipboard;
        struct String macro;
        struct edit* history;
        char* fileName;
        int cursor;
        int anchor;
        int cols;
        int rows;
        int commandCount;
        int undoCount;
        int redoCount;
        enum mode mode;
        enum record record;
};

struct lute l = { 0 };
struct termios initTermios;

//==============================================================
// select
//==============================================================

int SelectCharLeft(char* text, int len, int cursor) {
        assert(len >= 0);
        assert(len >= cursor);
        assert(cursor >= 0);
        if (text == NULL) return cursor;
        if (cursor == 0) return cursor;
        --cursor;
        if (text[cursor] == '\n') {
                ++cursor;
        }
        return cursor;
}

int SelectCharRight(char* text, int len, int cursor) {
        assert(len >= 0);
        assert(len >= cursor);
        assert(cursor >= 0);
        if (text == NULL) return cursor;
        if (cursor >= len - 1) return cursor;
        if (text[cursor] == '\n') return cursor;
        return ++cursor;
}

int SelectWordLeft(char* text, int len, int cursor) {
        assert(len >= 0);
        assert(len >= cursor);
        assert(cursor >= 0);
        if (text == NULL) return cursor;
        while (true) {
                if (cursor <= 0) return cursor;
                --cursor;
                if (text[cursor] == '\n') {
                        ++cursor;
                        break;
                }
                if (text[cursor] != ' ') break;
        }
        while (true) {
                if (cursor <= 0) return cursor;
                --cursor;
                if (text[cursor] == '\n') {
                        ++cursor;
                        break;
                }
                if (text[cursor] == ' ') {
                        ++cursor;
                        break;
                }
        }
        return cursor;
}

int SelectWordRight(char* text, int len, int cursor) {
        assert(len >= 0);
        assert(len >= cursor);
        assert(cursor >= 0);
        if (text == NULL) return cursor;
        while (true) {
                if (cursor >= len - 1) break;
                if (text[cursor] == '\n') break;
                if (text[cursor] == ' ') break;
                ++cursor;
        }
        while (true) {
                if (cursor >= len - 1) break;
                if (text[cursor] == '\n') break;
                if (text[cursor] != ' ') break;
                ++cursor;
        }
        return cursor;
}

int SelectLineStart(char* text, int len, int cursor) {
        assert(len >= 0);
        assert(len >= cursor);
        assert(cursor >= 0);
        if (text == NULL) return cursor;
        while (true) {
                if (cursor <= 0) break;
                --cursor;
                if (text[cursor] == '\n') {
                        ++cursor;
                        break;
                }
        }
        return cursor;
}

int SelectLineEnd(char* text, int len, int cursor) {
        assert(len >= 0);
        assert(len >= cursor);
        assert(cursor >= 0);
        if (text == NULL) return cursor;
        while (true) {
                if (cursor >= len - 1) break;
                if (text[cursor] == '\n') break;
                ++cursor;
        }
        return cursor;
}

int SelectLineUp(char* text, int len, int cursor) {
        assert(len >= 0);
        assert(len >= cursor);
        assert(cursor >= 0);
        if (text == NULL) return cursor;
        while (true) {
                if (cursor <= 0) return cursor;
                --cursor;
                if (text[cursor] == '\n') break;
        }
        while (true) {
                if (cursor <= 0) break;
                --cursor;
                if (text[cursor] == '\n') {
                        ++cursor;
                        break;
                }
        }
        return cursor;
}

int SelectLineDown(char* text, int len, int cursor) {
        assert(len >= 0);
        assert(len >= cursor);
        assert(cursor >= 0);
        if (text == NULL) return cursor;
        while (true) {
                if (cursor >= len - 1) break;
                if (text[cursor] == '\n') {
                        if (cursor >= len - 1) break;
                        ++cursor;
                        break;
                }
                ++cursor;
        }
        return cursor;
}

int SelectParaUp(char* text, int len, int cursor) {
        assert(len >= 0);
        assert(len >= cursor);
        assert(cursor >= 0);
        if (text == NULL) return cursor;
        while (true) {
                if (cursor <= 0) break;
                --cursor;
                if (text[cursor] == '\n') {
                        if (cursor <= 0) break;
                        if (cursor >= len - 1) break;
                        --cursor;
                        if (text[cursor] == '\n') {
                                ++cursor;
                                break;
                        }
                }
        }
        return cursor;
}

int SelectParaDown(char* text, int len, int cursor) {
        assert(len >= 0);
        assert(len >= cursor);
        assert(cursor >= 0);
        if (text == NULL) return cursor;
        while (true) {
                if (cursor >= len - 1) break;
                if (text[cursor] == '\n') {
                        if (cursor >= len - 1) break;
                        ++cursor;
                        if (text[cursor] == '\n') {
                                break;
                        }
                }
                ++cursor;
        }
        return cursor;
}

int SelectFindPrev(char* text, int len, int cursor, char find) {
        assert(len >= 0);
        assert(len >= cursor);
        assert(cursor >= 0);
        if (text == NULL) return cursor;
        while (true) {
                if (cursor <= 0) break;
                --cursor;
                if (text[cursor] == find) break;
        }
        return cursor;
}

int SelectFindNext(char* text, int len, int cursor, char find) {
        assert(len >= 0);
        assert(len >= cursor);
        assert(cursor >= 0);
        if (text == NULL) return cursor;
        while (true) {
                if (cursor >= len - 1) break;
                ++cursor;
                if (text[cursor] == find) break;
        }
        return cursor;
}

int SelectLineNumber(char* text, int len, int line) {
        assert(text != NULL);
        assert(len > 0);
        assert(line >= 0);
        int cursor = 0;
        if (text == NULL) return cursor;
        while (line > 1) {
                if (cursor >= len - 1) break;
                if (text[cursor] == '\n') {
                        --line;
                }
                ++cursor;
        }
        return cursor;
}

void SelectLineAll() {
        if (l.cursor >= l.anchor) {
                l.anchor = SelectLineStart(l.file.data, l.file.len, l.anchor);
                l.cursor = SelectLineEnd(l.file.data, l.file.len, l.cursor);
                ++l.cursor;
        } else {
                l.cursor = SelectLineStart(l.file.data, l.file.len, l.cursor);
                l.anchor = SelectLineEnd(l.file.data, l.file.len, l.anchor);
                ++l.anchor;
        }
}

size_t SelectLower() {
        return (l.cursor > l.anchor ? l.anchor : l.cursor);
}

size_t SelectHigher() {
        return (l.cursor > l.anchor ? l.cursor : l.anchor);
}

size_t SelectLen() {
        return SelectHigher() - SelectLower();
}

//==============================================================
// undo
//==============================================================

void UndoNewUndo() {
        if (l.undoCount > 0 && l.history[l.undoCount - 1].insert.len == 0 && l.history[l.undoCount - 1].delete.len == 0) {
                return;
        }
        for (; l.redoCount > 0; --l.redoCount) {
                StringFree(&l.history[l.undoCount + l.redoCount - 1].insert);
                StringFree(&l.history[l.undoCount + l.redoCount - 1].delete);
        }
        ++l.undoCount;
        struct edit* tmp = realloc(l.history, l.undoCount * sizeof(struct edit));
        assert(tmp != NULL);
        l.redoCount = 0;
        tmp[l.undoCount - 1].insert.data = NULL;
        tmp[l.undoCount - 1].insert.cap = 0;
        tmp[l.undoCount - 1].insert.len = 0;
        tmp[l.undoCount - 1].delete.data = NULL;
        tmp[l.undoCount - 1].delete.cap = 0;
        tmp[l.undoCount - 1].delete.len = 0;
        tmp[l.undoCount - 1].cursor = l.cursor;
        l.history = tmp;
}

void UndoExecuteUndo() {
        if (l.undoCount == 0) return;
        l.cursor = l.history[l.undoCount - 1].cursor;
        l.anchor = l.history[l.undoCount - 1].delete.len + l.cursor;
        StringDelete(&l.file, l.cursor, l.history[l.undoCount - 1].insert.len);
        StringInsert(&l.file, l.cursor, l.history[l.undoCount - 1].delete.data, l.history[l.undoCount - 1].delete.len);
        --l.undoCount;
        ++l.redoCount;
}

void UndoExecuteRedo() {
        if (l.redoCount == 0) return;
        l.cursor = l.history[l.undoCount].cursor;
        l.anchor = l.history[l.undoCount].insert.len + l.cursor;
        StringDelete(&l.file, l.cursor, l.history[l.undoCount].delete.len);
        StringInsert(&l.file, l.cursor, l.history[l.undoCount].insert.data, l.history[l.undoCount].insert.len);
        --l.redoCount;
        ++l.undoCount;
}

void UndoInsert(char* src, int count) {
        assert(src != NULL);
        assert(count >= 0);
        assert(l.redoCount == 0);
        StringAppend(&l.history[l.undoCount - 1].insert, src, count);
}

void UndoDelete(size_t count) {
        assert(l.redoCount == 0);
        if (l.history[l.undoCount - 1].insert.len >= count) {
                StringDelete(&l.history[l.undoCount - 1].insert, l.history[l.undoCount - 1].insert.len - count, count);
        } else if (l.history[l.undoCount - 1].insert.len == 0) {
                StringInsert(&l.history[l.undoCount - 1].delete, 0, &l.file.data[SelectLower()], count);
                l.history[l.undoCount - 1].cursor = SelectLower();
        } else {
                assert(false);
        }
}

//==============================================================
// control
//==============================================================

void PrintHelp() {
        printf("Usage: lute [options|file]\n\nOptions:\n");
        printf("        -h  Print the help message and exit\n");
        printf("        -k  Print the current keymap and exit\n");
}

void PrintKeymap() {
        printf("+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-----------+\n");
        printf("|       |       |       |       |       |       |       |swap ac|sel all|       |       |       |       |           |\n");
        printf("|       |       |       |       |       |       |       |       |       |       |       |       |       |           |\n");
        printf("|       |   1   |   2   |   3   |   4   |   5   |   6   |   7   |   8   |   9   |   0   |       |       |           |\n");
        printf("+-------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+-------+\n");
        printf("|           |   quit|       |1      |2      |       |2      |   redo| anchor|  above| anchor|       |       |       |\n");
        printf("|           |       |       |w next |replace|       |yank   |       |edit   |open   |paste  |       |       |       |\n");
        printf("|           |write q|  write|       |       |       |       |   undo| cursor|  below| cursor|       |       |       |\n");
        printf("+-----------+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+--+----+-------+\n");
        printf("|              |       |       |2      |       |1      |1      |1      |1      |1      | record|       |            |\n");
        printf("|              |       |       |delete |       |goto   |c prev |l down |l up   |c next |macro  |       |            |\n");
        printf("|              |       |       |       |       |       |       |       |       |       |execute|       |            |\n");
        printf("+--------------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+----------------+\n");
        printf("|                  |1      |1      |2      |       |1      |1      |1      |1      |1      |       |                |\n");
        printf("|                  |l start|l end  |change |       |w back |p down |p up   |f prev |f next |       |                |\n");
        printf("|                  |       |       |       |       |       |       |       |       |       |       |                |\n");
        printf("+------------------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+----------------+\n\n");
        printf("[1] = default changes selection; SHIFT extends selection\n");
        printf("[2] = default effects selection; SHIFT effects line\n");
}

void LoadArgs(int argc, char** argv) {
        if (argc != 2) {
                PrintHelp();
                exit(0);
        }
        if (strcmp(argv[1], "-h") == 0) {
                PrintHelp();
                exit(0);
        }
        if (strcmp(argv[1], "-k") == 0) {
                PrintKeymap();
                exit(0);
        }
        l.fileName = argv[1];
}

void DisableRawMode() {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &initTermios);
        write(STDOUT_FILENO, END_ALT_SCREEN, sizeof(END_ALT_SCREEN));
}

void EnableRawMode() {
        write(STDOUT_FILENO, START_ALT_SCREEN, sizeof(START_ALT_SCREEN));
        tcgetattr(STDIN_FILENO, &initTermios);
        struct termios rawTermios = initTermios;
        rawTermios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        rawTermios.c_cflag |= CS8;
        rawTermios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawTermios);
        atexit(DisableRawMode);
}

void LoadFile() {
        FILE* file = fopen(l.fileName, "r");
        if (file == NULL) {
                printf("Could not open file\n");
                exit(1);
        }
        fseek(file, 0L, SEEK_END);
        ssize_t length = ftell(file);
        fseek(file, 0L, SEEK_SET);
        if ( length < 0) {
                printf("Could not read file\n");
                exit(1);
        } else if ( length > 0) {
                assert( StringAlloc( &l.file, length + 1 ) );
                fread(l.file.data, length, 1, file);
                l.file.len = length;
        } else if ( length == 0) {
                assert( StringAppend( &l.file, "\n", 1 ) );
        } else {
                assert( false );
        }
        fclose(file);
}

void LoadScreen() {
        struct winsize winsize;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsize);
        l.cols = winsize.ws_col;
        l.rows = winsize.ws_row;
}

void WriteFile() {
        FILE* file = fopen(l.fileName, "w");
        assert(file != NULL);
        fwrite(l.file.data, sizeof(*l.file.data), l.file.len, file);
        fclose(file);
}

void DrawScreen(struct String* print) {
        char screen[l.cols * l.rows];
        size_t drawIndex = l.cursor;
        int drawLine = l.rows / 2;
        int drawCol = 0;
        int drawHighlightStart = 0;
        int drawHighlightEnd = l.cols * l.rows;
        for (int i = 0; i < l.cols * l.rows; ++i) {
                screen[i] = ' ';
        }
        if (SelectLineStart(l.file.data, l.file.len, drawIndex) == 0) {
                drawIndex = 0;
        } else {
                while (true) {
                        if (drawLine <= 0) break;
                        if (drawIndex <= 0) break;
                        drawIndex = SelectLineUp(l.file.data, l.file.len, drawIndex);
                        --drawLine;
                }
        }
        for (int i = 0; i < drawLine; ++i) {
                screen[i * l.cols] = '~';
        }
        while (drawLine < l.rows) {
                if (drawIndex == SelectLower()) {
                        drawHighlightStart = drawLine * l.cols + drawCol;
                }
                if (drawIndex == SelectHigher()) {
                        drawHighlightEnd = drawLine * l.cols + drawCol;
                }
                if (drawIndex >= l.file.len) break;
                if (drawCol >= l.cols) {
                        drawIndex = SelectLineDown(l.file.data, l.file.len, drawIndex);
                }
                if (l.file.data[drawIndex] == '\n') {
                        drawCol = 0;
                        ++drawIndex;
                        ++drawLine;
                        continue;
                }
                screen[drawLine * l.cols + drawCol] = l.file.data[drawIndex];
                ++drawCol;
                ++drawIndex;
        }
        while (drawLine < l.rows) {
                screen[drawLine * l.cols] = '~';
                ++drawLine;
        }
        assert( StringInsert(print, print->len, screen, l.rows * l.cols ) );
        assert( StringInsert(print, drawHighlightEnd, HIGHLIGHT_END, sizeof(HIGHLIGHT_END) ) );
        assert( StringInsert(print, drawHighlightStart, HIGHLIGHT_START, sizeof(HIGHLIGHT_START) ) );
}

void DrawCursor(struct String* print) {
        char cursorMove[27] = { 0 };
        sprintf(cursorMove, "\x1b[%d;%dH", l.rows / 2 + 1, l.cursor - SelectLineStart(l.file.data, l.file.len, l.cursor) + 1);
        assert( StringInsert(print, print->len, cursorMove, sizeof(cursorMove) ) );
}

void DrawFrame() {
        struct String print = { 0 };
        DrawScreen(&print);
        assert( StringInsert(&print, 0, CURSOR_HOME, sizeof(CURSOR_HOME) ) );
        assert( StringInsert(&print, 0, CURSOR_HIDDEN, sizeof(CURSOR_HIDDEN) ) );
        DrawCursor(&print);
        assert( StringInsert(&print, print.len, CURSOR_SHOW, sizeof(CURSOR_SHOW) ) );
        write(STDOUT_FILENO, print.data, print.len);
        StringFree(&print);
}

char GetInput() {
        char key = 0;
        read(STDIN_FILENO, &key, sizeof(char));
        if (key == LINEFEED_KEY) key = NEWLINE_KEY;
        return key;
}

void ProsessEdit(char key) {
        if (key == ESCAPE_KEY) {
                l.mode = COMMAND_MODE;
                l.anchor = l.cursor;
        } else if (key == DELETE_KEY) {
                if (l.cursor == 0) return;
                --l.cursor;
                UndoDelete(1);
                StringDelete(&l.file, l.cursor, 1);
                l.anchor = l.cursor;
                if (l.clipboard.len > 0) {
                        StringDelete(&l.clipboard, l.clipboard.len - 1, 1);
                }
        } else if (key == NEWLINE_KEY || (key >= SPACE_KEY && key < DELETE_KEY)) {
                StringInsert(&l.file, l.cursor, &key, 1);
                ++l.cursor;
                l.anchor = l.cursor;
                StringInsert(&l.clipboard, l.clipboard.len, &key, 1);
                UndoInsert(&key, 1);
        } else if (key == TAB_KEY) {
                char* tab = "        ";
                StringInsert(&l.file, l.cursor, tab, 8);
                l.cursor += 8;
                l.anchor = l.cursor;
                StringInsert(&l.clipboard, l.clipboard.len, tab, 8);
                UndoInsert(tab, 8);
        }
}

void ProsessSelect(int (*Call)(char*, int, int), bool extend) {
        if (l.commandCount == 0) {
                l.commandCount = 1;
        }
        while (l.commandCount > 0) {
                if (extend == false) {
                        l.anchor = l.cursor;
                }
                l.cursor = Call(l.file.data, l.file.len, l.cursor);
                --l.commandCount;
        }
}

void ProsessSelectFind(int (*Call)(char*, int, int, char), char key, bool extend) {
        if (key == ESCAPE_KEY) {
                l.commandCount = 0;
                return;
        }
        if (l.commandCount == 0) {
                l.commandCount = 1;
        }
        while (l.commandCount > 0) {
                if (extend == false) {
                        l.anchor = l.cursor;
                }
                l.cursor = Call(l.file.data, l.file.len, l.cursor, key);
                --l.commandCount;
        }
}

void ClipboardSelection() {
        StringFree(&l.clipboard);
        StringInsert(&l.clipboard, l.clipboard.len, &l.file.data[SelectLower()], SelectLen());
}

void DeleteSelection() {
        if (SelectLen() == 0) return;
        UndoNewUndo();
        UndoDelete(SelectLen());
        StringDelete(&l.file, SelectLower(), SelectLen());
        l.cursor = SelectLower();
        l.anchor = l.cursor;
}

void PasteSelection() {
        UndoNewUndo();
        StringInsert(&l.file, l.cursor, l.clipboard.data, l.clipboard.len);
        UndoInsert(&l.file.data[l.cursor], l.clipboard.len);
        l.cursor += l.clipboard.len;
}

void EnterEditMode() {
        assert(l.cursor == l.anchor);
        StringFree(&l.clipboard);
        l.commandCount = 0;
        l.mode = EDIT_MODE;
        UndoNewUndo();
}

void ProsessInput(char key);

void ExecuteMacro() {
        for (size_t i = 0; i < l.macro.len; ++i) {
                ProsessInput(l.macro.data[i]);
        }
}

void ProsessCommand(char key) {
        if (key >= '0' && key <= '9') {
                l.commandCount *= 10;
                l.commandCount += key & 0xf;
        } else if (key == 'w') {
                WriteFile();
        } else if (key == 'q') {
                WriteFile();
                exit(0);
        } else if (key == 'Q') {
                exit(0);
        } else if (key == 'h') {
                ProsessSelect(SelectCharLeft, false);
        } else if (key == 'H') {
                ProsessSelect(SelectCharLeft, true);
        } else if (key == 'l') {
                ProsessSelect(SelectCharRight, false);
        } else if (key == 'L') {
                ProsessSelect(SelectCharRight, true);
        } else if (key == 'b') {
                ProsessSelect(SelectWordLeft, false);
        } else if (key == 'B') {
                ProsessSelect(SelectWordLeft, true);
        } else if (key == 'e') {
                ProsessSelect(SelectWordRight, false);
        } else if (key == 'E') {
                ProsessSelect(SelectWordRight, true);
        } else if (key == 'z') {
                ProsessSelect(SelectLineStart, false);
        } else if (key == 'Z') {
                ProsessSelect(SelectLineStart, true);
        } else if (key == 'x') {
                ProsessSelect(SelectLineEnd, false);
        } else if (key == 'X') {
                ProsessSelect(SelectLineEnd, true);
        } else if (key == 'k') {
                ProsessSelect(SelectLineUp, false);
        } else if (key == 'K') {
                ProsessSelect(SelectLineUp, true);
        } else if (key == 'j') {
                ProsessSelect(SelectLineDown, false);
        } else if (key == 'J') {
                ProsessSelect(SelectLineDown, true);
        } else if (key == 'm') {
                ProsessSelect(SelectParaUp, false);
        } else if (key == 'M') {
                ProsessSelect(SelectParaUp, true);
        } else if (key == 'n') {
                ProsessSelect(SelectParaDown, false);
        } else if (key == 'N') {
                ProsessSelect(SelectParaDown, true);
        } else if (key == ',') {
                ProsessSelectFind(SelectFindPrev, GetInput(), false);
        } else if (key == '<') {
                ProsessSelectFind(SelectFindPrev, GetInput(), true);
        } else if (key == '.') {
                ProsessSelectFind(SelectFindNext, GetInput(), false);
        } else if (key == '>') {
                ProsessSelectFind(SelectFindNext, GetInput(), true);
        } else if (key == 'g') {
                l.anchor = l.cursor;
                l.cursor = SelectLineNumber(l.file.data, l.file.len, l.commandCount);
                l.commandCount = 0;
        } else if (key == 'G') {
                l.cursor = SelectLineNumber(l.file.data, l.file.len, l.commandCount);
                l.commandCount = 0;
        } else if (key == 'i') {
                l.anchor = l.cursor;
                EnterEditMode();
        } else if (key == 'I') {
                l.cursor = l.anchor;
                EnterEditMode();
        } else if (key == 'o') {
                char newline = '\n';
                l.cursor = SelectLineEnd(l.file.data, l.file.len, l.cursor);
                l.anchor = l.cursor;
                EnterEditMode();
                ProsessEdit(newline);
        } else if (key == 'O') {
                char newline = '\n';
                l.cursor = SelectLineStart(l.file.data, l.file.len, l.cursor);
                l.anchor = l.cursor;
                EnterEditMode();
                ProsessEdit(newline);
                --l.cursor;
        } else if (key == 'd') {
                ClipboardSelection();
                DeleteSelection();
        } else if (key == 'D') {
                SelectLineAll();
                ClipboardSelection();
                DeleteSelection();
        } else if (key == 'c') {
                ClipboardSelection();
                DeleteSelection();
                EnterEditMode();
        } else if (key == 'C') {
                SelectLineAll();
                ClipboardSelection();
                DeleteSelection();
                EnterEditMode();
        } else if (key == 'r') {
                DeleteSelection();
                PasteSelection();
        } else if (key == 'R') {
                SelectLineAll();
                DeleteSelection();
                PasteSelection();
        } else if (key == 'y') {
                ClipboardSelection();
        } else if (key == 'Y') {
                SelectLineAll();
                ClipboardSelection();
        } else if (key == 'p') {
                if (l.clipboard.data == NULL) return;
                l.anchor = l.cursor;
                PasteSelection();
        } else if (key == 'P') {
                if (l.clipboard.data == NULL) return;
                l.cursor = l.anchor;
                PasteSelection();
        } else if (key == 'u') {
                UndoExecuteUndo();
        } else if (key == 'U') {
                UndoExecuteRedo();
        } else if (key == ';') {
                if (l.record == RECORD_OFF) {
                        ExecuteMacro();
                } else {
                        l.record = RECORD_OFF;
                }
        } else if (key == ':') {
                if (l.record == RECORD_OFF) {
                        StringFree(&l.macro);
                        l.record = RECORD_ON;
                } else {
                        l.record = RECORD_OFF;
                }
        } else if (key == '&') {
                l.cursor = l.cursor ^ l.anchor;
                l.anchor = l.cursor ^ l.anchor;
                l.cursor = l.cursor ^ l.anchor;
        } else if (key == '*') {
                l.cursor = 0;
                l.anchor = l.file.len - 1;
        }
}

void ProsessInput(char key) {
        if (l.record == RECORD_ON) {
                if (key != ':' && key != ';') {
                        StringInsert(&l.macro, l.macro.len, &key, 1);
                }
        }
        if (l.mode == EDIT_MODE) {
                ProsessEdit(key);
        } else if (l.mode == COMMAND_MODE) {
                ProsessCommand(key);
        }
}

int main(int argc, char** argv) {
        LoadArgs(argc, argv);
        LoadFile();
        EnableRawMode();
        while (true) {
                LoadScreen();
                DrawFrame();
                char key = GetInput();
                ProsessInput(key);
        }
        return 0;
}
