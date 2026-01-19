#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "le.h"

#define START_ALT_SCREEN "\x1b[?1049h"
#define END_ALT_SCREEN "\x1b[?1049l"
#define ERASE_SCREEN "\x1b[2J"
#define CURSOR_HOME "\x1b[1;1H"

#define NEWLINE_KEY 13
#define ESCAPE_KEY 27
#define SPACE_KEY 32
#define DELETE_KEY 127

enum mode {
        NORMAL,
        INSERT,
};

enum command {
        MOVE,
        DELETE,
        CHANGE,
        YEET,
};

struct le {
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

struct le le = { 0 };
struct termios initTermios;

void LoadArgs(int argc, char** argv) {
        assert(argc == 2);
        le.fileName = argv[1];
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
        le.screenCols = winsize.ws_col;
        le.screenRows = winsize.ws_row;
        EnableRawMode();
}

void LoadFile() {
        size_t size = 0;
        ssize_t res = 0;
        FILE* file = fopen(le.fileName, "r");
        assert(file != NULL);
        res = getdelim(&le.text, &size, '\0', file);
        assert(le.text != NULL);
        assert(res != -1);
        le.textLen = res;
        fclose(file);
}

void LoadCommand() {
        le.command = MOVE;
        le.count = 0;
        le.mode = NORMAL;
}

void WriteFile() {
        FILE* file = fopen(le.fileName, "w");
        assert(file != NULL);
        fwrite(le.text, sizeof(*le.text), le.textLen, file);
        fclose(file);
}

void DrawLine(char* dst, char* src, int max) {
        while (max > 0 && *src != '\n') {
                *dst++ = *src++;
        }
}

void DrawFrame() {
        int frameLen = le.screenCols * le.screenRows + 1;
        int cursorRow = le.screenRows / 2;
        int startLineIndex = le.index + StartLineIndex(&le.text, le.textLen, le.index);
        char frame[frameLen];
        for (int i = 0; i < frameLen; ++i) {
                frame[i] = ' ';
        }
        for (int i = 0; i < le.screenRows; ++i) {
                if (i == cursorRow) {
                        DrawLine(&frame[le.screenCols * i], &le.text[startLineIndex], le.screenCols);
                } else if (i > cursorRow) {
                        int index = NextLineIndex(&le.text, le.textLen, le.index, i - cursorRow);
                        DrawLine(&frame[le.screenCols * i], &le.text[le.index + index], le.screenCols);
                } else {
                        frame[le.screenCols * i] = '~';
                }
        }
        frame[frameLen - 1] = '\0';
        char cursorMove[27] = { 0 };
        sprintf(cursorMove, "\x1b[%d;%dH", cursorRow + 1, le.index - startLineIndex + 1);
        write(STDOUT_FILENO, ERASE_SCREEN, sizeof(ERASE_SCREEN));
        write(STDOUT_FILENO, CURSOR_HOME, sizeof(CURSOR_HOME));
        write(STDOUT_FILENO, frame, frameLen);
        write(STDOUT_FILENO, cursorMove, sizeof(cursorMove));
}

void GetInput() {
        char key = 0;
        int move = 0;
        read(STDIN_FILENO, &key, sizeof(char));
        if (le.mode == INSERT) {
                if (key == ESCAPE_KEY) {
                        le.mode = NORMAL;
                } else if (key == DELETE_KEY) {
                        DeleteChars(&le.text, &le.textLen, le.index, 1);
                        --le.index;
                } else if (key == NEWLINE_KEY || (key >= SPACE_KEY && key < DELETE_KEY)) {
                        InsertChars(&le.text, &le.textLen, le.index, &key, 1);
                        ++le.index;
                }
        } else if (le.mode == NORMAL) {
                if        (key >= '0' && key <= '9') {
                        le.count *= 10;
                        le.count += key & 0xf;
                } else if (key == 'd') {
                        le.command = DELETE;
                } else if (key == 'c') {
                        le.command = CHANGE;
                } else if (key == 'y') {
                        le.command = YEET;
                } else if (key == 'q') {
                        exit(0);
                } else if (key == 'w') {
                        WriteFile();
                } else if (key == 'i') {
                        le.mode = INSERT;
                } else if (key == 'h') {
                        move = PrevCharIndex(&le.text, le.textLen, le.index, ((le.count < 1) ? 1 : le.count));
                } else if (key == 'l') {
                        move = NextCharIndex(&le.text, le.textLen, le.index, ((le.count < 1) ? 1 : le.count));
                } else if (key == 'k') {
                        move = PrevLineIndex(&le.text, le.textLen, le.index, ((le.count < 1) ? 1 : le.count));
                } else if (key == 'j') {
                        move = NextLineIndex(&le.text, le.textLen, le.index, ((le.count < 1) ? 1 : le.count));
                }
        }
        if (move != 0) {
                if (le.command == MOVE) {
                        le.index += move;
                } else if (le.command == DELETE) {
                } else if (le.command == CHANGE) {
                } else if (le.command == YEET) {
                }
                le.command = MOVE;
                le.count = 0;
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
