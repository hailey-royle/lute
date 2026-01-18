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
        ssize_t colCount;
        ssize_t rowCount;
};

struct buffer {
        char* text;
        ssize_t textCap;
        ssize_t textCount;
        ssize_t index;
};

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
        struct screen screen;
        struct buffer buffer;
        char* fileName;
        ssize_t count;
        enum mode mode;
        enum command command;
};

struct le le = { 0 };
struct termios initTermios;

void LoadArgs(int argc, char* argv[]) {
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
        //rawTermios.c_oflag &= ~(OPOST);
        rawTermios.c_cflag |= CS8;
        rawTermios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawTermios);
        atexit(DisableRawMode);
}

void LoadScreen() {
        struct winsize winsize;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsize);
        le.screen.colCount = winsize.ws_col;
        le.screen.rowCount = winsize.ws_row;
        EnableRawMode();
}

void LoadFile() {
        size_t size = 0;
        ssize_t res = 0;
        FILE* file = fopen(le.fileName, "r");
        assert(file != NULL);
        res = getdelim(&le.buffer.text, &size, '\0', file);
        assert(res != -1);
        le.buffer.textCap = res;
        le.buffer.textCount = res;
        fclose(file);
}

void LoadCommand() {
        le.command = MOVE;
        le.count = 1;
        le.mode = NORMAL;
}

void WriteFile() {
        FILE* file = fopen(le.fileName, "w");
        assert(file != NULL);
        fwrite(le.buffer.text, sizeof(*le.buffer.text), le.buffer.textCount, file);
        fclose(file);
}

void AppendFrame(char** frame, ssize_t* frameLength, const char* append, ssize_t appendLength) {
        char* tmp = realloc(*frame, *frameLength + appendLength);
        assert(tmp != NULL);
        memcpy(&tmp[*frameLength], append, appendLength);
        *frame = tmp;
        *frameLength += appendLength;
}

ssize_t StartLine() {
        int ret = 0;
        while (le.buffer.index + ret > 0) {
                if (le.buffer.text[le.buffer.index + ret - 1] == '\n') {
                        break;
                }
                --ret;
        }
        return ret;
}

void DrawFrame() {
        char* frame;
        ssize_t frameLength = 0;
        AppendFrame(&frame, &frameLength, ERASE_SCREEN, sizeof(ERASE_SCREEN));
        AppendFrame(&frame, &frameLength, CURSOR_HOME, sizeof(CURSOR_HOME));
        AppendFrame(&frame, &frameLength, le.buffer.text, le.buffer.textCount * sizeof(char));
        char cursorMove[45] = { 0 };
        sprintf(cursorMove, "\x1b[%ld;%ldH", (le.screen.rowCount / 2) + 1, ~(StartLine() - 1) + 1); 
        AppendFrame(&frame, &frameLength, cursorMove, strlen(cursorMove) * sizeof(char));
        write(STDOUT_FILENO, frame, frameLength * sizeof(char));
        free(frame);
}

void InsertChar(char insert) {
        if (le.buffer.textCap <= le.buffer.textCount) {
                le.buffer.textCap += TEXT_ALLOC_STEP;
                le.buffer.text = realloc(le.buffer.text, le.buffer.textCap * sizeof(*le.buffer.text));
                assert(le.buffer.text != NULL);
        }
        le.buffer.text[le.buffer.textCount] = '\0';
        ++le.buffer.textCount;
        memmove(&le.buffer.text[le.buffer.index + 1], &le.buffer.text[le.buffer.index], le.buffer.textCount - le.buffer.index);
        if (insert == NEWLINE_KEY) {
                le.buffer.text[le.buffer.index] = '\n';
        } else {
                le.buffer.text[le.buffer.index] = insert;
        }
        ++le.buffer.index;
}

void DeleteChar() {
        if (le.buffer.index <= 0) {
                return;
        }
        memmove(&le.buffer.text[le.buffer.index - 1], &le.buffer.text[le.buffer.index], le.buffer.textCount - le.buffer.index);
        --le.buffer.textCount;
        --le.buffer.index;
}

void ModeInsert() {
        le.mode = INSERT;
}

void ModeNormal() {
        le.mode = NORMAL;
}

ssize_t PrevChar(ssize_t count) {
        assert(count >= 0);
        if (count > le.buffer.index) {
                return ~(le.buffer.index - 1);
        } else {
                return ~(count - 1);
        }
}

ssize_t NextChar(ssize_t count) {
        assert(count >= 0);
        if (count <= le.buffer.textCount - le.buffer.index) {
                return count;
        } else {
                return le.buffer.textCount - le.buffer.index;
        }
}

ssize_t PrevLine(ssize_t count) {
        assert(count >= 0);
        ssize_t ret = 0;
        while (count > 0) {
                if (ret + 1 >= le.buffer.index) {
                        ret = le.buffer.index;
                        break;
                }
                if (le.buffer.text[le.buffer.index - ret] == '\n') {
                        --count;
                }
                ++ret;
        }
//        while (le.buffer.index + ret > 0) {
//                if (le.buffer.text[le.buffer.index + ret - 1] == '\n') {
//                        break;
//                }
//                ++ret;
//        }
        return ~(ret - 1);
}

ssize_t NextLine(ssize_t count) {
        assert(count >= 0);
        ssize_t ret = 0;
        while (count > 0) {
                if (ret >= le.buffer.textCount - le.buffer.index) {
                        ret = le.buffer.textCount - le.buffer.index;
                        break;
                }
                if (le.buffer.text[le.buffer.index + ret] == '\n') {
                        --count;
                }
                ++ret;
        }
        return ret;
}

void GetInput() {
        char key = 0;
        ssize_t move = 0;
        read(STDIN_FILENO, &key, sizeof(char));
        if (le.mode == INSERT) {
                if (key == ESCAPE_KEY) {
                        ModeNormal();
                } else if (key == DELETE_KEY) {
                        DeleteChar();
                } else if (key == NEWLINE_KEY || (key >= SPACE_KEY && key < DELETE_KEY)) {
                        InsertChar(key);
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
                        ModeInsert();
                } else if (key == 'h') {
                        move = PrevChar(le.count);
                } else if (key == 'l') {
                        move = NextChar(le.count);
                } else if (key == 'k') {
                        move = PrevLine(le.count);
                } else if (key == 'j') {
                        move = NextLine(le.count);
                }
        }
        if (move != 0) {
                if (le.command == MOVE) {
                        le.buffer.index += move;
                } else if (le.command == DELETE) {
                } else if (le.command == CHANGE) {
                } else if (le.command == YEET) {
                }
                le.command = MOVE;
                le.count = 1;
                move = 0;
        }
}

int main(int argc, char* argv[]) {
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
