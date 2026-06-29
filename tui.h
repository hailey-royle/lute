#ifndef TUI
#define TUI

#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "assert.h"

#define BACKSPACE_KEY 8
#define TAB_KEY 9
#define NEWLINE_KEY 10
#define LINEFEED_KEY 13
#define ESCAPE_KEY 27
#define DELETE_KEY 127

#define CURSOR_HOME "\x1b[H"
#define ERASE_LINE "\x1b[2K"

#define CURSOR_SHOW "\x1b[?25h"
#define CURSOR_HIDDEN "\x1b[?25l"
#define START_ALT_SCREEN "\x1b[?1049h"
#define END_ALT_SCREEN "\x1b[?1049l"

#define RESET_GRAPHICS "\x1b[0m"
#define INVERSE_START "\x1b[7m"
#define INVERSE_END "\x1b[27m"

#define BACKGROUND_BLACK "\x1b[40m"
#define FOREGROUND_BLACK "\x1b[30m"
#define FOREGROUND_RED "\x1b[31m"
#define BACKGROUND_RED "\x1b[41m"
#define FOREGROUND_GREEN "\x1b[32m"
#define BACKGROUND_GREEN "\x1b[42m"
#define FOREGROUND_YELLOW "\x1b[33m"
#define BACKGROUND_YELLOW "\x1b[43m"
#define FOREGROUND_BLUE "\x1b[34m"
#define BACKGROUND_BLUE "\x1b[44m"
#define FOREGROUND_MAGENTA "\x1b[35m"
#define BACKGROUND_MAGENTA "\x1b[45m"
#define FOREGROUND_CYAN "\x1b[36m"
#define BACKGROUND_CYAN "\x1b[46m"
#define FOREGROUND_WHITE "\x1b[37m"
#define BACKGROUND_WHITE "\x1b[47m"
#define FOREGROUND_DEFAULT "\x1b[39m"
#define BACKGROUND_DEFAULT "\x1b[49m"

#define INPUT_BUFFER_CAP 4096

struct Input{
        char data[ INPUT_BUFFER_CAP ];
        size_t len;
        size_t index;
};

struct Input input = { 0 };

struct termios initTermios = { 0 };

void DisableRawMode(){
        int err = tcsetattr( STDIN_FILENO, TCSAFLUSH, &initTermios );
        Assert( err != -1, "tcsetattr failed." );
        write( STDOUT_FILENO, END_ALT_SCREEN, sizeof( END_ALT_SCREEN ));
        write( STDOUT_FILENO, CURSOR_SHOW, sizeof( CURSOR_SHOW ) );
}

void EnableRawMode(){
        write( STDOUT_FILENO, START_ALT_SCREEN, sizeof( START_ALT_SCREEN ));
        write( STDOUT_FILENO, CURSOR_HIDDEN, sizeof( CURSOR_HIDDEN ));
        int err = tcgetattr( STDIN_FILENO, &initTermios );
        Assert( err != -1, "tcgetattr failed." );
        struct termios rawTermios = initTermios;
//        rawTermios.c_oflag &= ~OPOST; // turns off /n into /r/n
        rawTermios.c_iflag &= ~( IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON );
        rawTermios.c_lflag &= ~( ECHO | ECHONL | ICANON | ISIG | IEXTEN );
        rawTermios.c_cflag &= ~( CSIZE | PARENB );
        rawTermios.c_cflag |= CS8;
        rawTermios.c_cc[ VMIN ] = 0;
        rawTermios.c_cc[ VTIME ] = 1;
        err = tcsetattr( STDIN_FILENO, TCSAFLUSH, &rawTermios );
        Assert( err != -1, "tcsetattr failed." );
        atexit( DisableRawMode );
}

void GetScreenSize( size_t* cols, size_t* rows ){
        struct winsize winsize;
        int err = ioctl( STDOUT_FILENO, TIOCGWINSZ, &winsize );
        Assert( err != -1, "ioctl failed." );
        *cols = winsize.ws_col;
        *rows = winsize.ws_row;
}

char GetInputBuffer(){
        char key = 0;
        if( input.len == 0 ){
                return key;
        }
        key = input.data[ input.index ];
        input.index++;
        if( input.index >= input.len ){
                input.index = 0;
                input.len = 0;
        }
        return key;
}

char GetInputBufferRead(){
        char key = 0;
        if( input.len == 0 ){
                Assert( input.index == 0 && input.len == 0, "input data malformed" );
                ssize_t bytes_read = read( STDIN_FILENO, input.data, INPUT_BUFFER_CAP );
                Assert( bytes_read >= 0, "read() failed." );
                input.len = ( size_t ) bytes_read;
                for( size_t i = 0; i < input.len; i++ ){
                        Assert( input.data[ i ] != '\0', "read error" );
                        if( input.data[ i ] == LINEFEED_KEY ){
                                input.data[ i ] = NEWLINE_KEY;
                        }
                }
                if( input.len == 0 ){
                        return key;
                }
        }
        key = input.data[ input.index ];
        input.index++;
        if( input.index >= input.len ){
                input.index = 0;
                input.len = 0;
        }
        return key;
}

#endif
