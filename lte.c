#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#define START_ALT_SCREEN "\x1b[?1049h"
#define END_ALT_SCREEN "\x1b[?1049l"
#define ERASE_SCREEN "\x1b[2J"
#define CURSOR_HOME "\x1b[1;1H"
#define START_HIGHLIGHT "\x1b[41m"
#define END_HIGHLIGHT "\x1b[49m"

#define TAB_KEY 9
#define NEWLINE_KEY 10
#define LINEFEED_KEY 13
#define ESCAPE_KEY 27
#define SPACE_KEY 32
#define DELETE_KEY 127

struct string {
        char* text;
        int len;
};

struct edit {
        struct string insert;
        struct string delete;
        int cursor;
};

enum mode {
        COMMAND_MODE,
        EDIT_MODE,
};

struct lte {
        struct string file;
        struct string clipboard;
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
};

struct lte lte = { 0 };
struct termios initTermios;

//==============================================================
// String
//==============================================================

void StringInsert(struct string* string, int index, char* src, int count) {
        assert(string != NULL);
        assert(string->len >= 0);
        assert(index >= 0);
        assert(count >= 0);
        if (src == NULL) return;
        if (count == 0) return;
        char* tmp = realloc(string->text, string->len + count);
        assert(tmp != NULL);
        memmove(&tmp[index + count], &tmp[index], string->len - index);
        memmove(&tmp[index], src, count);
        string->text = tmp;
        string->len += count;
}

void StringDelete(struct string* string, int index, int count) {
        assert(string != NULL);
        assert(string->text != NULL);
        assert(string->len >= index + count);
        if (count == 0) return;
        memmove(&string->text[index], &string->text[index + count], string->len - count - index);
        string->text = realloc(string->text, string->len - count);
        if (string->len - count != 0) {
                assert(string->text != NULL);
        }
        string->len -= count;
}

void StringErase(struct string* string) {
        assert(string != NULL);
        if (string->text == NULL) return;
        free(string->text);
        string->text = NULL;
        string->len = 0;
}

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

int SelectLineNumber(char* text, int len, int line) {
        assert(text != NULL);
        assert(len > 0);
        assert(line >= 0);
        int cursor = 0;
        if (text == NULL) return cursor;
        while (line > 0) {
                if (cursor >= len - 1) break;
                if (text[cursor] == '\n') {
                        --line;
                }
                ++cursor;
        }
        return cursor;
}

void SelectLineAll() {
        if (lte.cursor >= lte.anchor) {
                lte.anchor = SelectLineStart(lte.file.text, lte.file.len, lte.anchor);
                lte.cursor = SelectLineEnd(lte.file.text, lte.file.len, lte.cursor);
                ++lte.cursor;
        } else {
                lte.cursor = SelectLineStart(lte.file.text, lte.file.len, lte.cursor);
                lte.anchor = SelectLineEnd(lte.file.text, lte.file.len, lte.anchor);
                ++lte.anchor;
        }
}

int SelectLower() {
        return (lte.cursor > lte.anchor ? lte.anchor : lte.cursor);
}

int SelectHigher() {
        return (lte.cursor > lte.anchor ? lte.cursor : lte.anchor);
}

int SelectLen() {
        return SelectHigher() - SelectLower();
}

//==============================================================
// undo
//==============================================================

void UndoNewUndo() {
        if (lte.undoCount > 0 && lte.history[lte.undoCount - 1].insert.len == 0 && lte.history[lte.undoCount - 1].delete.len == 0) {
                return;
        }
        for (; lte.redoCount > 0; --lte.redoCount) {
                StringErase(&lte.history[lte.undoCount + lte.redoCount - 1].insert);
                StringErase(&lte.history[lte.undoCount + lte.redoCount - 1].delete);
        }
        ++lte.undoCount;
        struct edit* tmp = realloc(lte.history, lte.undoCount * sizeof(struct edit));
        assert(tmp != NULL);
        lte.redoCount = 0;
        tmp[lte.undoCount - 1].insert.text = NULL;
        tmp[lte.undoCount - 1].insert.len = 0;
        tmp[lte.undoCount - 1].delete.text = NULL;
        tmp[lte.undoCount - 1].delete.len = 0;
        tmp[lte.undoCount - 1].cursor = lte.cursor;
        lte.history = tmp;
}

void UndoExecuteUndo() {
        if (lte.undoCount == 0) return;
        lte.cursor = lte.history[lte.undoCount - 1].cursor;
        lte.anchor = lte.history[lte.undoCount - 1].delete.len + lte.cursor;
        StringDelete(&lte.file, lte.cursor, lte.history[lte.undoCount - 1].insert.len);
        StringInsert(&lte.file, lte.cursor, lte.history[lte.undoCount - 1].delete.text, lte.history[lte.undoCount - 1].delete.len);
        --lte.undoCount;
        ++lte.redoCount;
}

void UndoExecuteRedo() {
        if (lte.redoCount == 0) return;
        lte.cursor = lte.history[lte.undoCount].cursor;
        lte.anchor = lte.history[lte.undoCount].insert.len + lte.cursor;
        StringDelete(&lte.file, lte.cursor, lte.history[lte.undoCount].delete.len);
        StringInsert(&lte.file, lte.cursor, lte.history[lte.undoCount].insert.text, lte.history[lte.undoCount].insert.len);
        --lte.redoCount;
        ++lte.undoCount;
}

void UndoInsert(char* src, int count) {
        assert(src != NULL);
        assert(count >= 0);
        assert(lte.redoCount == 0);
        StringInsert(&lte.history[lte.undoCount - 1].insert, lte.history[lte.undoCount - 1].insert.len, src, count);
}

void UndoDelete(int count) {
        assert(count >= 0);
        assert(lte.redoCount == 0);
        if (lte.history[lte.undoCount - 1].insert.len >= count) {
                StringDelete(&lte.history[lte.undoCount - 1].insert, lte.history[lte.undoCount - 1].insert.len - count, count);
        } else if (lte.history[lte.undoCount - 1].insert.len == 0) {
                StringInsert(&lte.history[lte.undoCount - 1].delete, 0, &lte.file.text[SelectLower()], count);
                lte.history[lte.undoCount - 1].cursor = SelectLower();
        } else {
                assert(false);
        }
}

//==============================================================
// control
//==============================================================

void LoadArgs(int argc, char** argv) {
        if (argc == 2) {
                lte.fileName = argv[1];
        } else {
                printf("Usage: lte [file]\n");
                exit(1);
        }
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
        FILE* file = fopen(lte.fileName, "r");
        if (file == NULL) {
                printf("Could not open file\n");
                exit(1);
        }
        fseek(file, 0L, SEEK_END);
        lte.file.len = ftell(file);
        fseek(file, 0L, SEEK_SET);
        if (lte.file.len < 0) {
                printf("Could not read file\n");
                exit(1);
        }
        if (lte.file.len > 0) {
                lte.file.text = realloc(lte.file.text, lte.file.len);
                assert(lte.file.text != NULL);
                fread(lte.file.text, lte.file.len, 1, file);
        }
        if (lte.file.len == 0) {
                lte.file.len = 1;
                lte.file.text = realloc(lte.file.text, lte.file.len);
                assert(lte.file.text != NULL);
                lte.file.text[0] = '\n';
        }
        fclose(file);
}

void LoadScreen() {
        struct winsize winsize;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsize);
        lte.cols = winsize.ws_col;
        lte.rows = winsize.ws_row;
        EnableRawMode();
}

void WriteFile() {
        FILE* file = fopen(lte.fileName, "w");
        assert(file != NULL);
        fwrite(lte.file.text, sizeof(*lte.file.text), lte.file.len, file);
        fclose(file);
}

void DrawBlankLines(struct string* print, int count) {
        char* newline = "~\n";
        while (count > 0) {
                StringInsert(print, print->len, newline, 2);
                --count;
        }
}

void DrawFile(struct string* print) {
        int index = lte.cursor;
        int screenLine = lte.rows / 2;
        int jump = 0;
        if (SelectLineStart(lte.file.text, lte.file.len, index) == 0) {
                index = 0;
        } else {
                while (true) {
                        if (screenLine <= 0) break;
                        if (index <= 0) break;
                        index = SelectLineUp(lte.file.text, lte.file.len, index);
                        --screenLine;
                }
        }
        DrawBlankLines(print, screenLine);
        if (index > SelectLower()) {
                StringInsert(print, print->len, START_HIGHLIGHT, sizeof(START_HIGHLIGHT));
        }
        while (true) {
                if (index + jump >= lte.file.len) break;
                if (lte.file.text[index + jump] == '\n') {
                        ++screenLine;
                        if (screenLine >= lte.rows) break;
                }
                if (index + jump == SelectLower()) {
                        StringInsert(print, print->len, &lte.file.text[index], jump);
                        StringInsert(print, print->len, START_HIGHLIGHT, sizeof(START_HIGHLIGHT));
                        index += jump;
                        jump = 0;
                }
                if (index + jump == SelectHigher()) {
                        StringInsert(print, print->len, &lte.file.text[index], jump);
                        StringInsert(print, print->len, END_HIGHLIGHT, sizeof(END_HIGHLIGHT));
                        index += jump;
                        jump = 0;
                }
                ++jump;
        }
        StringInsert(print, print->len, &lte.file.text[index], jump);
        if (index + jump < SelectHigher()) {
                StringInsert(print, print->len, END_HIGHLIGHT, sizeof(END_HIGHLIGHT));
        }
        if (lte.rows - screenLine > 0) {
                char tilde = '~';
                DrawBlankLines(print, lte.rows - screenLine - 1);
                StringInsert(print, print->len, &tilde, 1);
        }
}

void DrawCursor(struct string* print) {
        char cursorMove[27] = { 0 };
        sprintf(cursorMove, "\x1b[%d;%dH", lte.rows / 2 + 1, lte.cursor - SelectLineStart(lte.file.text, lte.file.len, lte.cursor) + 1);
        StringInsert(print, print->len, cursorMove, sizeof(cursorMove));
}

void DrawFrame() {
        struct string print;
        print.text = NULL;
        print.len = 0;
        StringInsert(&print, print.len, CURSOR_HOME, sizeof(CURSOR_HOME));
        StringInsert(&print, print.len, ERASE_SCREEN, sizeof(ERASE_SCREEN));
        DrawFile(&print);
        DrawCursor(&print);
        write(STDOUT_FILENO, print.text, print.len);
        StringErase(&print);
}

void ProsessEdit(char key) {
        if (key == ESCAPE_KEY) {
                lte.mode = COMMAND_MODE;
                lte.anchor = lte.cursor;
        } else if (key == DELETE_KEY) {
                if (lte.cursor == 0) return;
                --lte.cursor;
                UndoDelete(1);
                StringDelete(&lte.file, lte.cursor, 1);
                lte.anchor = lte.cursor;
                if (lte.clipboard.len > 0) {
                        StringDelete(&lte.clipboard, lte.clipboard.len - 1, 1);
                }
        } else if (key == NEWLINE_KEY || (key >= SPACE_KEY && key < DELETE_KEY)) {
                StringInsert(&lte.file, lte.cursor, &key, 1);
                ++lte.cursor;
                lte.anchor = lte.cursor;
                StringInsert(&lte.clipboard, lte.clipboard.len, &key, 1);
                UndoInsert(&key, 1);
        } else if (key == TAB_KEY) {
                char* tab = "        ";
                StringInsert(&lte.file, lte.cursor, tab, 8);
                lte.cursor += 8;
                lte.anchor = lte.cursor;
                StringInsert(&lte.clipboard, lte.clipboard.len, tab, 8);
                UndoInsert(tab, 8);
        }
}

void ProsessSelect(int (*Call)(char*, int, int), bool extend) {
        if (lte.commandCount == 0) {
                lte.commandCount = 1;
        }
        while (lte.commandCount > 0) {
                if (extend == false) {
                        lte.anchor = lte.cursor;
                }
                lte.cursor = Call(lte.file.text, lte.file.len, lte.cursor);
                --lte.commandCount;
        }
}

void ClipboardSelection() {
        StringErase(&lte.clipboard);
        StringInsert(&lte.clipboard, lte.clipboard.len, &lte.file.text[SelectLower()], SelectLen());
}

void DeleteSelection() {
        if (SelectLen() == 0) return;
        UndoNewUndo();
        UndoDelete(SelectLen());
        StringDelete(&lte.file, SelectLower(), SelectLen());
        lte.cursor = SelectLower();
        lte.anchor = lte.cursor;
}

void PasteSelection() {
        UndoNewUndo();
        UndoInsert(&lte.file.text[lte.cursor], lte.clipboard.len);
        StringInsert(&lte.file, lte.cursor, lte.clipboard.text, lte.clipboard.len);
        lte.cursor += lte.clipboard.len;
}

void EnterEditMode() {
        assert(lte.cursor == lte.anchor);
        StringErase(&lte.clipboard);
        lte.commandCount = 0;
        lte.mode = EDIT_MODE;
        UndoNewUndo();
}

void ProsessCommand(char key) {
        if (key >= '0' && key <= '9') {
                lte.commandCount *= 10;
                lte.commandCount += key & 0xf;
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
        } else if (key == 'g') {
                lte.anchor = lte.cursor;
                lte.cursor = SelectLineNumber(lte.file.text, lte.file.len, lte.commandCount);
                lte.commandCount = 0;
        } else if (key == 'G') {
                lte.cursor = SelectLineNumber(lte.file.text, lte.file.len, lte.commandCount);
                lte.commandCount = 0;
        } else if (key == 'i') {
                lte.anchor = lte.cursor;
                EnterEditMode();
        } else if (key == 'I') {
                lte.cursor = lte.anchor;
                EnterEditMode();
        } else if (key == 'o') {
                char newline = '\n';
                lte.cursor = SelectLineEnd(lte.file.text, lte.file.len, lte.cursor);
                lte.anchor = lte.cursor;
                EnterEditMode();
                ProsessEdit(newline);
        } else if (key == 'O') {
                char newline = '\n';
                lte.cursor = SelectLineStart(lte.file.text, lte.file.len, lte.cursor);
                lte.anchor = lte.cursor;
                EnterEditMode();
                ProsessEdit(newline);
                --lte.cursor;
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
                if (lte.clipboard.text == NULL) return;
                lte.anchor = lte.cursor;
                PasteSelection();
        } else if (key == 'P') {
                if (lte.clipboard.text == NULL) return;
                lte.cursor = lte.anchor;
                PasteSelection();
        } else if (key == 'u') {
                UndoExecuteUndo();
        } else if (key == 'U') {
                UndoExecuteRedo();
        }
}

void GetInput() {
        char key = 0;
        read(STDIN_FILENO, &key, sizeof(char));
        if (key == LINEFEED_KEY) key = NEWLINE_KEY;
        if (lte.mode == EDIT_MODE) {
                ProsessEdit(key);
        } else if (lte.mode == COMMAND_MODE) {
                ProsessCommand(key);
        }
}

int main(int argc, char** argv) {
        LoadArgs(argc, argv);
        LoadFile();
        LoadScreen();
        while (true) {
                DrawFrame();
                GetInput();
        }
        return 0;
}
