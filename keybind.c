#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>

typedef int32_t i32;
typedef uint8_t u8;

i32 main( i32 argc, char* argv[] ){
	struct termios init_termios;
	i32 err = tcgetattr( STDIN_FILENO, &init_termios );
	if( err != 0 ){
		error( "This terminal is not supported. (Unable to enter raw mode)" );
	}
	struct termios raw_termios = init_termios;
	raw_termios.c_iflag &= ~( IGNBRK | BRKINT | PARMRK | ISTRIP | IXON );
	raw_termios.c_lflag &= ~( ECHO | ECHONL | ICANON | IEXTEN | ISIG );
	raw_termios.c_cflag &= ~( CSIZE | PARENB );
	raw_termios.c_cflag |= ( CS8 );
	err = tcsetattr( STDIN_FILENO, TCSAFLUSH, &raw_termios );
	if( err != 0 ){
		error( "This terminal is not supported. (Unable to enter raw mode)" );
	}
	printf( "Type 'q' to quit.\n" );
	i32 input_count = 16;
	u8 input[ input_count ];
	while( 1 ){
		i32 bytes_read = read( STDIN_FILENO, input, input_count );
		printf( "\"" );
		i32 i = 0;
		while( i < bytes_read ){
			if( input[ i ] < 32 || input[ i ] >= 127 ){
				printf( "\\x%0*x", 2, input[ i ]);
			} else {
				printf( "%c", input[ i ]);
			}
			i += 1;
		}
		printf( "\"\n" );
		if( input[ 0 ] == 'q' ){
			break;
		}
	}
	tcsetattr( STDIN_FILENO, TCSAFLUSH, &init_termios );
	return 0;
}
