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

#define LINE_ALLOC_STEP 256
#define TEXT_ALLOC_STEP 1024
#define INPUT_MAX 32

#define NEWLINE_KEY 13
#define ESCAPE_KEY 27
#define SPACE_KEY 32
#define DELETE_KEY 127

struct screen {
        char* text;
        ssize_t colCount;
        ssize_t rowCount;
};

struct line {
        ssize_t start;
        ssize_t length;
};

struct data {
        struct line* line;
        char* text;
        ssize_t textCap;
        ssize_t textCount;
        ssize_t lineCap;
        ssize_t lineCount;
        ssize_t index;
};

enum mode {
        NORMAL,
        INSERT,
};

struct le {
        char input[INPUT_MAX];
        struct screen s;
        struct data d;
        char* fileName;
        ssize_t inputCount;
        enum mode mode;
};

struct le l = { 0 };
struct termios initTermios;

void LoadArgs(int argc, char* argv[]) {
        assert(argc == 2);
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
        rawTermios.c_oflag &= ~(OPOST);
        rawTermios.c_cflag &= ~(CS8);
        rawTermios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawTermios);
        atexit(DisableRawMode);
}

void LoadScreen() {
        struct winsize winsize;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsize);
        l.s.colCount = winsize.ws_col;
        l.s.rowCount = winsize.ws_row;
        l.s.text = malloc(l.s.colCount * l.s.rowCount * sizeof(*l.s.text));
        assert(l.s.text != NULL);
        EnableRawMode();
}

void UpdateLines() {
        assert(l.d.text != NULL);
        int index = 0;
        l.d.lineCount = 0;
        while (true) {
                if (l.d.lineCap <= l.d.lineCount) {
                        l.d.lineCap += LINE_ALLOC_STEP;
                        l.d.line = realloc(l.d.line, l.d.lineCap * sizeof(struct line));
                        assert(l.d.line != NULL);
                }
                assert(l.d.line != NULL);
                l.d.line[l.d.lineCount].start = index;
                while(l.d.text[index] != '\n') {
                        ++index;
                }
                ++index;
                l.d.line[l.d.lineCount].length = index - l.d.line[l.d.lineCount].start;
                ++l.d.lineCount;
                if (l.d.text[index] == '\0') {
                        break;
                }
        }
}

void LoadFile() {
        size_t size = 0;
        ssize_t res = 0;
        FILE* file = fopen(l.fileName, "r");
        assert(file != NULL);
        res = getdelim(&l.d.text, &size, '\0', file);
        assert(res != -1);
        l.d.textCap = res;
        l.d.textCount = res;
        fclose(file);
        UpdateLines();
}

void WriteFile() {
        FILE* file = fopen(l.fileName, "w");
        assert(file != NULL);
        fwrite(l.d.text, sizeof(*l.d.text), l.d.textCount, file);
        fclose(file);
}

ssize_t GetLineNumber() {
        ssize_t i = 0;
        while (l.d.index > l.d.line[i].start + l.d.line[i].length - 1) {
                ++i;
        }
        return i;
}

void DrawLine(char* dst, ssize_t max, ssize_t lineNumber) {
        if (lineNumber < 0 || lineNumber > l.d.lineCount - 1) {
                return;
        }
        if (l.d.line[lineNumber].length < max) {
                max = l.d.line[lineNumber].length;
        }
        snprintf(dst, max, "%s", &l.d.text[l.d.line[lineNumber].start]);
        dst[max - 1] = ' ';
}

void DrawFrame() {
        char cursorMove[45] = {'\0'};
        for (ssize_t i = 0; i < l.s.rowCount * l.s.colCount; ++i) {
                l.s.text[i] = ' ';
        }
        for (ssize_t i = 0; i < l.s.rowCount; ++i) {
                DrawLine(&l.s.text[l.s.colCount * i], l.s.colCount, i + GetLineNumber() - (l.s.rowCount / 2));
        }
        sprintf(cursorMove, "\x1b[%ld;%ldH", (l.s.rowCount / 2) + 1, l.d.index - l.d.line[GetLineNumber()].start + 1); 
        write(STDOUT_FILENO, ERASE_SCREEN, sizeof(ERASE_SCREEN));
        write(STDOUT_FILENO, CURSOR_HOME, sizeof(CURSOR_HOME));
        write(STDOUT_FILENO, l.s.text, l.s.colCount * l.s.rowCount * sizeof(*l.s.text));
        write(STDOUT_FILENO, cursorMove, strlen(cursorMove));
}

void InsertChar(char insert) {
        if (l.d.textCap <= l.d.textCount) {
                l.d.textCap += TEXT_ALLOC_STEP;
                l.d.text = realloc(l.d.text, l.d.textCap * sizeof(*l.d.text));
                assert(l.d.text != NULL);
        }
        l.d.text[l.d.textCount] = '\0';
        ++l.d.textCount;
        memmove(&l.d.text[l.d.index + 1], &l.d.text[l.d.index], l.d.textCount - l.d.index);
        if (insert == NEWLINE_KEY) {
                l.d.text[l.d.index] = '\n';
        } else {
                l.d.text[l.d.index] = insert;
        }
        ++l.d.index;
        UpdateLines();
}

void DeleteChar() {
        if (l.d.index <= 0) {
                return;
        }
        memmove(&l.d.text[l.d.index - 1], &l.d.text[l.d.index], l.d.textCount - l.d.index);
        --l.d.textCount;
        --l.d.index;
        UpdateLines();
}

void ModeInsert() {
        l.mode = INSERT;
}

void ModeNormal() {
        l.mode = NORMAL;
}

void MovePrevCursor() {
        if (l.d.index > 0) {
                --l.d.index;
        }
}

void MoveNextCursor() {
        if (l.d.index < l.d.textCount - 1) {
                ++l.d.index;
        }
}

void MovePrevLine() {
        ssize_t lineNumber = GetLineNumber();
        if (lineNumber > 0) {
                l.d.index = l.d.line[lineNumber - 1].start;
        }
}

void MoveNextLine() {
        ssize_t lineNumber = GetLineNumber();
        if (lineNumber < l.d.lineCount - 1) {
                l.d.index = l.d.line[lineNumber + 1].start;
        }
}

void GetInput() {
        read(STDIN_FILENO, &l.input[l.inputCount], sizeof(char));
        if (l.mode == NORMAL) {
                if        (l.input[l.inputCount] == 'q') {
                        exit(0);
                } else if (l.input[l.inputCount] == 'w') {
                        WriteFile();
                } else if (l.input[l.inputCount] == 'i') {
                        ModeInsert();
                } else if (l.input[l.inputCount] == 'h') {
                        MovePrevCursor();
                } else if (l.input[l.inputCount] == 'l') {
                        MoveNextCursor();
                } else if (l.input[l.inputCount] == 'k') {
                        MovePrevLine();
                } else if (l.input[l.inputCount] == 'j') {
                        MoveNextLine();
                }
        } else if (l.mode == INSERT) {
                if (l.input[l.inputCount] == ESCAPE_KEY) {
                        ModeNormal();
                } else if (l.input[l.inputCount] == DELETE_KEY) {
                        DeleteChar();
                } else if (l.input[l.inputCount] == NEWLINE_KEY || (l.input[l.inputCount] >= SPACE_KEY && l.input[l.inputCount] < DELETE_KEY)) {
                        InsertChar(l.input[l.inputCount]);
                }
        }
}

int main(int argc, char* argv[]) {
        LoadArgs(argc, argv);
        LoadScreen();
        LoadFile();
        while (true) {
                DrawFrame();
                GetInput();
        }
        return 0;
}
