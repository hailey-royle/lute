#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "lte.h"

#define START_ALT_SCREEN "\x1b[?1049h"
#define END_ALT_SCREEN "\x1b[?1049l"
#define ERASE_SCREEN "\x1b[2J"
#define CURSOR_HOME "\x1b[1;1H"

#define NEWLINE_KEY 10
#define LINEFEED_KEY 13
#define ESCAPE_KEY 27
#define SPACE_KEY 32
#define DELETE_KEY 127

enum mode {
        COMMAND,
        EDIT,
};

enum command {
        MOVE,
        DELETE,
        CHANGE,
        YEET,
};

struct lte {
        char* text;
        char* fileName;
        int textLen;
        int index;
        int screenCols;
        int screenRows;
        int count;
        enum mode mode;
        enum command command;
};

struct lte lte = { 0 };
struct termios initTermios;

void LoadArgs(int argc, char** argv) {
        assert(argc == 2);
        lte.fileName = argv[1];
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

void LoadScreen() {
        struct winsize winsize;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsize);
        lte.screenCols = winsize.ws_col;
        lte.screenRows = winsize.ws_row;
        EnableRawMode();
}

void LoadFile() {
        size_t size = 0;
        ssize_t res = 0;
        FILE* file = fopen(lte.fileName, "r");
        assert(file != NULL);
        res = getdelim(&lte.text, &size, '\0', file);
        assert(lte.text != NULL);
        assert(res != -1);
        lte.textLen = ++res;
        fclose(file);
}

void LoadCommand() {
        lte.command = MOVE;
        lte.count = 0;
        lte.mode = COMMAND;
}

void WriteFile() {
        FILE* file = fopen(lte.fileName, "w");
        assert(file != NULL);
        fwrite(lte.text, sizeof(*lte.text), lte.textLen - 1, file);
        fclose(file);
}

void DrawLine(char* dst, char* src, int max) {
        while (max > 0 && *src != '\n' && *src != '\0') {
                *dst++ = *src++;
                --max;
        }
}

void DrawFrame() {
        int frameLen = lte.screenCols * lte.screenRows + 1;
        int cursorRow = lte.screenRows / 2;
        int startLineIndex = lte.index + StartLineIndex(&lte.text, lte.textLen, lte.index);
        int lineNumber = LineNumber(&lte.text, lte.textLen, lte.index);
        char frame[frameLen];
        char cursorMove[27] = { 0 };
        char print[frameLen + sizeof(ERASE_SCREEN) + sizeof(CURSOR_HOME) + sizeof(cursorMove)];
        print[0] = '\0';
        for (int i = 0; i < frameLen; ++i) {
                frame[i] = ' ';
        }
        for (int i = 0; i < lte.screenRows; ++i) {
                if (i == cursorRow) {
                        DrawLine(&frame[lte.screenCols * cursorRow], &lte.text[startLineIndex], lte.screenCols);
                } else if (i > cursorRow) {
                        int index = NextLineIndex(&lte.text, lte.textLen, lte.index, i - cursorRow);
                        DrawLine(&frame[lte.screenCols * i], &lte.text[lte.index + index], lte.screenCols);
                } else if (i < cursorRow && ~(i - cursorRow) + 1 <= lineNumber) {
                        int index = PrevLineIndex(&lte.text, lte.textLen, lte.index, ~(i - cursorRow) + 1);
                        DrawLine(&frame[lte.screenCols * i], &lte.text[lte.index + index], lte.screenCols);
                }
        }
        frame[frameLen - 1] = '\0';
        sprintf(cursorMove, "\x1b[%d;%dH", cursorRow + 1, lte.index - startLineIndex + 1);
        strcat(print, CURSOR_HOME);
        strcat(print, ERASE_SCREEN);
        strcat(print, frame);
        strcat(print, cursorMove);
        write(STDOUT_FILENO, print, strlen(print));
}

void GetInput() {
        char key = 0;
        int move = 0;
        read(STDIN_FILENO, &key, sizeof(char));
        if (key == LINEFEED_KEY) {
                key = NEWLINE_KEY;
        }
        if (lte.mode == EDIT) {
                if (key == ESCAPE_KEY) {
                        lte.mode = COMMAND;
                } else if (key == DELETE_KEY) {
                        lte.command = MOVE;
                        lte.count = 0;
                        --lte.index;
                        DeleteChars(&lte.text, &lte.textLen, lte.index, 1);
                } else if (key == NEWLINE_KEY || (key >= SPACE_KEY && key < DELETE_KEY)) {
                        lte.command = MOVE;
                        lte.count = 0;
                        InsertChars(&lte.text, &lte.textLen, lte.index, &key, 1);
                        ++lte.index;
                }
        } else if (lte.mode == COMMAND) {
                if        (key >= '0' && key <= '9') {
                        lte.count *= 10;
                        lte.count += key & 0xf;
                } else if (key == 'd') {
                        lte.command = DELETE;
                } else if (key == 'c') {
                        lte.command = CHANGE;
                } else if (key == 'y') {
                        lte.command = YEET;
                } else if (key == 'q') {
                        exit(0);
                } else if (key == 'w') {
                        WriteFile();
                } else if (key == 'i') {
                        lte.mode = EDIT;
                } else if (key == 'a') {
                        move = NextCharIndex(&lte.text, lte.textLen, lte.index, ((lte.count < 1) ? 1 : lte.count));
                        lte.mode = EDIT;
                } else if (key == 'h') {
                        move = PrevCharIndex(&lte.text, lte.textLen, lte.index, ((lte.count < 1) ? 1 : lte.count));
                } else if (key == 'l') {
                        move = NextCharIndex(&lte.text, lte.textLen, lte.index, ((lte.count < 1) ? 1 : lte.count));
                } else if (key == 'b') {
                        move = PrevWordIndex(&lte.text, lte.textLen, lte.index, ((lte.count < 1) ? 1 : lte.count));
                } else if (key == 'e') {
                        move = NextWordIndex(&lte.text, lte.textLen, lte.index, ((lte.count < 1) ? 1 : lte.count));
                } else if (key == 'k') {
                        move = PrevLineIndex(&lte.text, lte.textLen, lte.index, ((lte.count < 1) ? 1 : lte.count));
                } else if (key == 'j') {
                        move = NextLineIndex(&lte.text, lte.textLen, lte.index, ((lte.count < 1) ? 1 : lte.count));
                } else if (key == 'n') {
                        move = PrevParaIndex(&lte.text, lte.textLen, lte.index, ((lte.count < 1) ? 1 : lte.count));
                } else if (key == 'm') {
                        move = NextParaIndex(&lte.text, lte.textLen, lte.index, ((lte.count < 1) ? 1 : lte.count));
                } else if (key == 'z') {
                        move = StartLineIndex(&lte.text, lte.textLen, lte.index);
                } else if (key == 'x') {
                        move = EndLineIndex(&lte.text, lte.textLen, lte.index);
                } else if (key == 'g') {
                        move = LineIndex(&lte.text, lte.textLen, lte.index, ((lte.count < 1) ? 1 : lte.count));
                }
        }
        if (move != 0) {
                if (lte.command == MOVE) {
                        lte.index += move;
                } else if (lte.command == DELETE) {
                        if (move < 0) {
                                lte.index += move;
                        }
                        DeleteChars(&lte.text, &lte.textLen, lte.index, abs(move));
                } else if (lte.command == CHANGE) {
                        if (move < 0) {
                                lte.index += move;
                        }
                        DeleteChars(&lte.text, &lte.textLen, lte.index, abs(move));
                        lte.mode = EDIT;
                } else if (lte.command == YEET) {
                }
                lte.command = MOVE;
                lte.count = 0;
        }
}

int main(int argc, char** argv) {
        LoadArgs(argc, argv);
        LoadScreen();
        LoadFile();
        LoadCommand();
        while (true) {
                DrawFrame();
                GetInput();
        }
        return 0;
}
