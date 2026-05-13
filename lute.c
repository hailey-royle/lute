#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "string.h"
#include "assert.h"

#define START_ALT_SCREEN "\x1b[?1049h"
#define END_ALT_SCREEN "\x1b[?1049l"
#define CURSOR_HIDDEN "\x1b[?25l"
#define CURSOR_SHOW "\x1b[?25h"
#define CURSOR_HOME "\x1b[1;1H"
#define HIGHLIGHT_START "\x1b[41m"
#define HIGHLIGHT_END "\x1b[49m"
#define INVERSE_START "\x1b[7m"
#define INVERSE_END "\x1b[27m"
#define ERASE_LINE "\x1b[2K"

#define TAB_KEY 9
#define NEWLINE_KEY 10
#define LINEFEED_KEY 13
#define ESCAPE_KEY 27
#define DELETE_KEY 127

#define INPUT_BUFFER_CAP 1024

struct Input{
	char data[ INPUT_BUFFER_CAP ];
	size_t len;
	size_t index;
};

struct Screen{
	size_t cols;
	size_t rows;
};

struct Selection{
	struct String clipboard;
	size_t cursor;
	size_t anchor;
};

struct SelectionArray{
	struct Selection* data;
	size_t count;
};

struct EditSelection{
	struct String insert;
	struct String delete;
	size_t index;
};

struct EditSelectionArray{
	struct EditSelection* data;
	size_t count;
};

struct EditArray{
	struct EditSelectionArray* data;
	size_t undo_count;
	size_t redo_count;
};

struct termios initTermios;

struct SelectionArray selection = { 0 };
struct EditArray edit = { 0 };
struct Screen screen = { 0 };
struct Input input = { 0 };
struct String file = { 0 };
char* file_name = NULL;
size_t command_count = 0;
bool edit_mode = false;
bool anchor_pinned = false;

void LoadArgs( int argc, char** argv ){
	if( argc != 2 ){
		printf( "Usage: lute <filename>\n" );
		exit( 0 );
	}
	file_name = argv[ 1 ];
}

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
//	rawTermios.c_oflag &= ~OPOST; // turns off /n into /r/n
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

void LoadScreen(){
	struct winsize winsize;
	int err = ioctl( STDOUT_FILENO, TIOCGWINSZ, &winsize );
	Assert( err != -1, "ioctl failed." );
	screen.cols = winsize.ws_col;
	screen.rows = winsize.ws_row;
}

size_t DrawLine( struct String* print, size_t start_index, bool* highlight ){
	size_t real_end_index = StringSelectLineEnd( &file, start_index ) + 1;
	size_t cliped_end_index = real_end_index;
	size_t real_line_length = real_end_index - start_index;
	size_t cliped_line_length = real_line_length;
	if( cliped_line_length >= screen.cols ){
		cliped_line_length = screen.cols;
		cliped_end_index = start_index + screen.cols;
	}
	if( *highlight == true ){
		StringAppend( print, HIGHLIGHT_START, sizeof( HIGHLIGHT_START ));
	}
	for( size_t i = start_index; i < cliped_end_index; i++ ){
		bool inverse_flag = false;
		for( size_t j = 0; j < selection.count; j++ ){
			if( selection.data[ j ].anchor == i ){
				if( *highlight == false ){
					StringAppend( print, HIGHLIGHT_START, sizeof( HIGHLIGHT_START ));
					*highlight = true;
				} else {
					StringAppend( print, HIGHLIGHT_END, sizeof( HIGHLIGHT_END ));
					*highlight = false;
				}
			}
			if( selection.data[ j ].cursor == i ){
				if( *highlight == false ){
					StringAppend( print, HIGHLIGHT_START, sizeof( HIGHLIGHT_START ));
					*highlight = true;
				} else {
					StringAppend( print, HIGHLIGHT_END, sizeof( HIGHLIGHT_END ));
					*highlight = false;
				}
				StringAppend( print, INVERSE_START, sizeof( INVERSE_START ));
				inverse_flag = true;
			}
		}
		if( file.data[ i ] == '\n' ){
			StringAppend( print, " ", 1 );
			if( inverse_flag ){
				StringAppend( print, INVERSE_END, sizeof( INVERSE_END ));
			}
			break;
		} else if( file.data[ i ] == '\t' ){
			StringAppend( print, "        ", 8 );
			if( cliped_line_length >= screen.cols - 7 ){
				cliped_line_length -= screen.cols - 7;
				cliped_end_index -= screen.cols - 7;
			}
			if( inverse_flag ){
				StringAppend( print, INVERSE_END, sizeof( INVERSE_END ));
			}
		} else {
			StringAppend( print, &file.data[ i ], 1 );
			if( inverse_flag ){
				StringAppend( print, INVERSE_END, sizeof( INVERSE_END ));
			}
		}
	}
	StringAppend( print, HIGHLIGHT_END, sizeof( HIGHLIGHT_END ));
	for( size_t i = cliped_end_index; i < real_end_index; i++ ){
		for( size_t j = 0; j < selection.count; j++ ){
			if( selection.data[ j ].anchor == i ){
				if( *highlight == false ){
					*highlight = true;
				} else {
					*highlight = false;
				}
			}
			if( selection.data[ j ].cursor == i ){
				if( *highlight == false ){
					*highlight = true;
				} else {
					*highlight = false;
				}
			}
		}
	}
	return real_end_index;
}

void DrawScreen(){
	struct String print = { 0 };
	bool highlight = false;
	int drawLine = 0;
	size_t drawIndex = StringSelectLineStart( &file, selection.data[ 0 ].cursor );
	for( size_t i = 0; i < screen.rows / 2; i++ ){
		if( drawIndex > 0 ){
			drawIndex = StringSelectLinePrev( &file, drawIndex );
		} else {
			drawLine--;
		}
	}
	StringAppend( &print, CURSOR_HOME, sizeof( CURSOR_HOME ));
	for( size_t j = 0; j < selection.count; j++ ){
		if( selection.data[ j ].anchor < drawIndex ){
			if( highlight == false ){
				highlight = true;
			} else {
				highlight = false;
			}
		}
		if( selection.data[ j ].cursor < drawIndex ){
			if( highlight == false ){
				highlight = true;
			} else {
				highlight = false;
			}
		}
	}
	for( size_t i = 0; i < screen.rows; i++ ){
		if( i != 0 ) {
			StringAppend( &print, "\r\n", 2 );
		}
		StringAppend( &print, ERASE_LINE, sizeof( ERASE_LINE ));
		if( drawLine < 0 || drawIndex >= file.len ){
			StringAppend( &print, "~", 1 );
			drawLine++;
		} else {
			drawIndex = DrawLine( &print, drawIndex, &highlight );
			drawLine++;
		}
	}
	write(STDOUT_FILENO, print.data, print.len);
	StringFree( &print );
}

void InputBufferRead(){
	Assert( input.index == 0 && input.len == 0, "input data malformed" );
	ssize_t bytes_read = read( STDIN_FILENO, input.data, INPUT_BUFFER_CAP );
	Assert( bytes_read >= 0, "read error" );
	input.len = ( size_t ) bytes_read;
	for( size_t i = 0; i < input.len; i++ ){
		Assert( input.data[ i ] != '\0', "read error" );
		if( input.data[ i ] == LINEFEED_KEY ){
			input.data[ i ] = NEWLINE_KEY;
		}
	}
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
		InputBufferRead();
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

char GetInputBufferWait(){
	char key = 0;
	while( input.len == 0 ){
		InputBufferRead();
	}
	key = input.data[ input.index ];
	input.index++;
	if( input.index >= input.len ){
		input.index = 0;
		input.len = 0;
	}
	return key;
}

void SelectionNew( size_t index ){
	selection.count++;
	selection.data = realloc( selection.data, selection.count * sizeof( selection.data[ 0 ]));
	Assert( selection.data != NULL, "Alloc failed." );
	selection.data[ selection.count - 1 ].cursor = index;
	selection.data[ selection.count - 1 ].anchor = index ;
	selection.data[ selection.count - 1 ].clipboard.data = NULL;
	selection.data[ selection.count - 1 ].clipboard.len = 0;
	selection.data[ selection.count - 1 ].clipboard.cap = 0;
}

void SelectionFree( size_t index ){
	Assert( index < selection.count, " " );
	StringFree( &selection.data[ index ].clipboard );
	for( size_t i = index; i < selection.count - 1; i++ ){
		selection.data[ i ].cursor = selection.data[ i + 1 ].cursor;
		selection.data[ i ].anchor = selection.data[ i + 1 ].anchor;
		selection.data[ i ].clipboard.data = selection.data[ i + 1 ].clipboard.data;
		selection.data[ i ].clipboard.len = selection.data[ i + 1 ].clipboard.len;
		selection.data[ i ].clipboard.cap = selection.data[ i + 1 ].clipboard.cap;
	}
	selection.count--;
}

void EditNew(){
	if( edit.undo_count > 0 ){
		for( size_t i = 0; i <= edit.data[ edit.undo_count - 1 ].count; i++ ){
			if( i == edit.data[ edit.undo_count - 1 ].count ){
				return;
			}
			if( edit.data[ edit.undo_count - 1 ].data[ i ].insert.len != 0 || edit.data[ edit.undo_count - 1 ].data[ i ].delete.len != 0 ){
				break;
			}
		}
	}
	for( size_t i = edit.undo_count; i < edit.undo_count + edit.redo_count; i++ ){
		for( size_t j = 0; j < edit.data[ i ].count; j++ ){
			StringFree( &edit.data[ i ].data[ j ].insert );
			StringFree( &edit.data[ i ].data[ j ].delete );
		}
		free( edit.data[ i ].data );
	}
	edit.undo_count++;
	edit.redo_count = 0;
	edit.data = realloc( edit.data, edit.undo_count * sizeof( edit.data[ 0 ]));
	Assert( edit.data != NULL, "Alloc failed." );
	struct EditSelectionArray* new_edit = &edit.data[ edit.undo_count - 1 ];
	new_edit->count = selection.count;
	new_edit->data = NULL;
	new_edit->data = realloc( new_edit->data, new_edit->count * sizeof( new_edit->data[ 0 ] ));
	Assert( new_edit->data != NULL, "Alloc failed." );
	memset( new_edit->data, 0, new_edit->count * sizeof( new_edit->data[ 0 ] ));
}

void EditUndo(){
	if( edit.undo_count <= 0 ){
		return;
	}
	struct EditSelectionArray* undo = &edit.data[ edit.undo_count - 1 ];
	while( selection.count < undo->count ){
		SelectionNew( 0 );
	}
	while( selection.count > undo->count ){
		SelectionFree( undo->count );
	}
	for( size_t i = 0; i < undo->count; i++ ){
		StringDelete( &file, undo->data[ i ].index, undo->data[ i ].insert.len );
		if( undo->data[ i ].delete.len > 0 ){
			StringInsert( &file, undo->data[ i ].index, undo->data[ i ].delete.data, undo->data[ i ].delete.len );
		}
		selection.data[ i ].cursor = undo->data[ i ].index + undo->data[ i ].delete.len;
		selection.data[ i ].anchor = undo->data[ i ].index;
		for( size_t j = 0; j < undo->count; j++ ){
			if( undo->data[ j ].index > undo->data[ i ].index ){
				undo->data[ j ].index -= undo->data[ i ].insert.len;
				undo->data[ j ].index += undo->data[ i ].delete.len;
				selection.data[ j ].cursor -= undo->data[ i ].insert.len;
				selection.data[ j ].cursor += undo->data[ i ].delete.len;
				selection.data[ j ].anchor -= undo->data[ i ].insert.len;
				selection.data[ j ].anchor += undo->data[ i ].delete.len;
			}
		}
	}
	edit.undo_count--;
	edit.redo_count++;
}

void EditRedo(){
	if( edit.redo_count <= 0 ){
		return;
	}
	struct EditSelectionArray* undo = &edit.data[ edit.undo_count ];
	while( selection.count < undo->count ){
		SelectionNew( 0 );
	}
	while( selection.count > undo->count ){
		SelectionFree( undo->count );
	}
	for( size_t i = 0; i < undo->count; i++ ){
		StringDelete( &file, undo->data[ i ].index, undo->data[ i ].delete.len );
		if( undo->data[ i ].insert.len > 0 ){
			StringInsert( &file, undo->data[ i ].index, undo->data[ i ].insert.data, undo->data[ i ].insert.len );
		}
		selection.data[ i ].cursor = undo->data[ i ].index + undo->data[ i ].insert.len;
		selection.data[ i ].anchor = undo->data[ i ].index;
		for( size_t j = 0; j < undo->count; j++ ){
			if( undo->data[ j ].index > undo->data[ i ].index ){
				undo->data[ j ].index += undo->data[ i ].insert.len;
				undo->data[ j ].index -= undo->data[ i ].delete.len;
				selection.data[ j ].cursor += undo->data[ i ].insert.len;
				selection.data[ j ].cursor -= undo->data[ i ].delete.len;
				selection.data[ j ].anchor += undo->data[ i ].insert.len;
				selection.data[ j ].anchor -= undo->data[ i ].delete.len;
			}
		}
	}
	edit.undo_count++;
	edit.redo_count--;
}

void EditModeInit(){
	edit_mode = true;
	for( size_t i = 0; i < selection.count; i++ ){
		StringFree( &selection.data[ i ].clipboard );
		selection.data[ i ].anchor = selection.data[ i ].cursor;
		edit.data[ edit.undo_count - 1 ].data[ i ].index = selection.data[ i ].cursor;
	}
}

void SelectCursorLine(){
	for( size_t i = 0; i < selection.count; i++ ){
		selection.data[ i ].cursor = StringSelectLineStart( &file, selection.data[ i ].cursor );
		selection.data[ i ].anchor = StringSelectLineNext( &file, selection.data[ i ].cursor );
	}
}

void DeleteSelection(){
	struct EditSelectionArray* undo = &edit.data[ edit.undo_count - 1 ];
	for( size_t i = 0; i < undo->count; i++ ){
		if( selection.data[ i ].cursor > selection.data[ i ].anchor ){
			size_t tmp = selection.data[ i ].anchor;
			selection.data[ i ].anchor = selection.data[ i ].cursor;
			selection.data[ i ].cursor = tmp;
		}
		undo->data[ i ].index = selection.data[ i ].cursor;
	}
	for( size_t i = 0; i < selection.count; i++ ){
		Assert( selection.data[ i ].anchor >= selection.data[ i ].cursor, "you fucked up" );
		size_t selection_len = selection.data[ i ].anchor - selection.data[ i ].cursor;
		for( size_t j = 0; j < selection.count; j++ ){
			if( selection.data[ j ].cursor > selection.data[ i ].cursor ){
				undo->data[ j ].index -= selection_len;
				selection.data[ j ].cursor -= selection_len;
				selection.data[ j ].anchor -= selection_len;
			}
		}
		if( undo->data[ i ].insert.len >= selection_len ){
			StringDeduct( &undo->data[ i ].insert, selection_len );
		} else if( undo->data[ i ].insert.len > 0 ){
			StringDeduct( &undo->data[ i ].insert, undo->data[ i ].insert.len );
			StringInsert( &undo->data[ i ].delete, 0, &file.data[ selection.data[ i ].cursor ], selection_len - undo->data[ i ].insert.len );
		} else {
			StringInsert( &undo->data[ i ].delete, 0, &file.data[ selection.data[ i ].cursor ], selection_len );
		}
		StringDelete( &file, selection.data[ i ].cursor, selection_len );
		selection.data[ i ].anchor = selection.data[ i ].cursor;
	}
}

void CopySelection(){
	for( size_t i = 0; i < selection.count; i++ ){
		StringFree( &selection.data[ i ].clipboard );
		if( selection.data[ i ].cursor > selection.data[ i ].anchor ){
			StringAppend( &selection.data[ i ].clipboard, &file.data[ selection.data[ i ].anchor ], selection.data[ i ].cursor - selection.data[ i ].anchor );
		} else if( selection.data[ i ].anchor > selection.data[ i ].cursor ){
			StringAppend( &selection.data[ i ].clipboard, &file.data[ selection.data[ i ].cursor ], selection.data[ i ].anchor - selection.data[ i ].cursor );
		}
	}
}

void PasteSelection(){
	struct EditSelectionArray* undo = &edit.data[ edit.undo_count - 1 ];
	for( size_t i = 0; i < undo->count; i++ ){
		undo->data[ i ].index = selection.data[ i ].cursor;
	}
	for( size_t i = 0; i < selection.count; i++ ){
		if( selection.data[ i ].clipboard.len == 0 || selection.data[ i ].clipboard.data ==  NULL ){
			continue;
		}
		for( size_t j = 0; j < selection.count; j++ ){
			if( selection.data[ j ].cursor > selection.data[ i ].cursor ){
				undo->data[ j ].index += selection.data[ i ].clipboard.len;
				selection.data[ j ].cursor += selection.data[ i ].clipboard.len;
				selection.data[ j ].anchor += selection.data[ i ].clipboard.len;
			}
		}
		StringAppend( &undo->data[ i ].insert, selection.data[ i ].clipboard.data, selection.data[ i ].clipboard.len ); 
		StringInsert( &file, selection.data[ i ].cursor, selection.data[ i ].clipboard.data, selection.data[ i ].clipboard.len );
		selection.data[ i ].anchor = selection.data[ i ].cursor;
		selection.data[ i ].cursor = selection.data[ i ].cursor + selection.data[ i ].clipboard.len;
	}
}

void ProsessEditInsert( char* key, size_t key_len ){
	Assert( key_len > 0, "Malformed arguments" );
	Assert( key != NULL, "Malformed arguments" );
	struct EditSelectionArray* undo = &edit.data[ edit.undo_count - 1 ];
	for( size_t i = 0; i < selection.count; i++ ){
		Assert( undo->count == selection.count, "you fucked up" );
		for( size_t j = 0; j < selection.count; j++ ){
			if( undo->data[ j ].index > undo->data[ i ].index ){
				undo->data[ j ].index += key_len;
				selection.data[ j ].cursor += key_len;
				selection.data[ j ].anchor = selection.data[ j ].cursor;
			}
		}
		StringAppend( &undo->data[ i ].insert, key, key_len );
		StringAppend( &selection.data[ i ].clipboard, key, key_len );
		StringInsert( &file, selection.data[ i ].cursor, key, key_len );
		selection.data[ i ].cursor += key_len;
		selection.data[ i ].anchor = selection.data[ i ].cursor;
	}
}

void ProsessEditDelete( size_t delete_len ){
	struct EditSelectionArray* undo = &edit.data[ edit.undo_count - 1 ];
	for( size_t i = 0; i < selection.count; i++ ){
		Assert( undo->count == selection.count, "you fucked up" );
		if( selection.data[ i ].cursor == 0 ){
			break;
		}
		selection.data[ i ].cursor -= delete_len ;
		selection.data[ i ].anchor = selection.data[ i ].cursor;
		for( size_t j = 0; j < selection.count; j++ ){
			if( undo->data[ j ].index > undo->data[ i ].index ){
				undo->data[ j ].index -= delete_len ;
				selection.data[ j ].cursor -= delete_len ;
				selection.data[ j ].anchor = selection.data[ j ].cursor;
			}
		}
		if( undo->data[ i ].insert.len > delete_len ){
			StringDeduct( &undo->data[ i ].insert, delete_len );
		} else if( undo->data[ i ].insert.len > 0 ){
			StringDeduct( &undo->data[ i ].insert, undo->data[ i ].insert.len );
			StringInsert( &undo->data[ i ].delete, 0, &file.data[ selection.data[ i ].cursor ], delete_len - undo->data[ i ].insert.len );
			undo->data[ i ].index -= delete_len - undo->data[ i ].insert.len;
		} else {
			StringInsert( &undo->data[ i ].delete, 0, &file.data[ selection.data[ i ].cursor ], delete_len );
			undo->data[ i ].index -= delete_len;
		}
		if( selection.data[ i ].clipboard.len > delete_len ){
			StringDeduct( &selection.data[ i ].clipboard, delete_len );
		} else if( selection.data[ i ].clipboard.len > 0 ){
			StringDeduct( &selection.data[ i ].clipboard, selection.data[ i ].clipboard.len );
		}
		StringDelete( &file, selection.data[ i ].cursor, delete_len );
	}
}

#define ProsessSelectionMove( func ){ \
	if( command_count == 0 ){ \
		command_count = 1; \
	} \
	for( size_t i = 0; i < selection.count; i++ ){ \
		for( size_t j = 0; j < command_count; j++ ){ \
			if( !anchor_pinned ){ \
				selection.data[ i ].anchor = selection.data[ i ].cursor; \
			} \
			selection.data[ i ].cursor = func( &file, selection.data[ i ].cursor ); \
		} \
	} \
	command_count = 0; \
}

#define ProsessSelectionMoveChar( func, dst ){ \
	if( command_count == 0 ){ \
		command_count = 1; \
	} \
	for( size_t i = 0; i < selection.count; i++ ){ \
		for( size_t j = 0; j < command_count; j++ ){ \
			if( !anchor_pinned ){ \
				selection.data[ i ].anchor = selection.data[ i ].cursor; \
			} \
			selection.data[ i ].cursor = func( &file, selection.data[ i ].cursor, dst ); \
		} \
	} \
	command_count = 0; \
}

#define ProsessSelectionInside( lower, upper, left, right ){ \
	for( size_t i = 0; i < selection.count; i++ ){ \
		size_t lower_index = selection.data[ i ].cursor; \
		size_t upper_index = selection.data[ i ].cursor - 1; \
		size_t j = 0; \
		while( true ){ \
			size_t left_index = StringSelectFindCharPrev( &file, lower_index, left ); \
			size_t right_index = StringSelectFindCharPrev( &file, lower_index, right ); \
			if( right_index <= left_index ){ \
				lower_index = left_index; \
				if( j == 0 ){ \
					break; \
				} \
				j--; \
			} else { \
				lower_index = right_index; \
				j++; \
			} \
		} \
		while( true ){ \
			size_t left_index = StringSelectFindCharNext( &file, upper_index, left ); \
			size_t right_index = StringSelectFindCharNext( &file, upper_index, right ); \
			if( right_index <= left_index ){ \
				upper_index = right_index; \
				if( j == 0 ){ \
					break; \
				} \
				j--; \
			} else { \
				upper_index = left_index; \
				j++; \
			} \
		} \
		if( file.data[ lower_index ] == left && file.data[ upper_index ] == right ){ \
			upper = upper_index + 1; \
			lower = lower_index; \
		} \
	} \
}

void ProsessCommand( char key ){
	if( key == 'q' ){
		StringToFile( &file, file_name );
		exit( 0 );
	} else if( key == 'Q' ){
		exit( 0 );
	} else if( key == 'w' ){
		command_count = 0;
		StringToFile( &file, file_name );
	} else if( key == 'i' ){
		command_count = 0;
		EditNew();
		EditModeInit();
	} else if( key == 'o' ){
		command_count = 0;
		ProsessSelectionMove( StringSelectLineEnd );
		EditNew();
		EditModeInit();
		ProsessEditInsert( "\n", 1 );
	} else if( key == 'u' ){
		command_count = 0;
		EditUndo();
	} else if( key == 'U' ){
		command_count = 0;
		EditRedo();
	} else if( key == 'a' ){
		command_count = 0;
		if( anchor_pinned ){ 
			anchor_pinned = false;
		} else {
			anchor_pinned = true;
		}
	} else if( key == 'A' ){
		command_count = 0;
		for( size_t i = 0; i < selection.count; i++ ){
			size_t tmp = selection.data[ i ].cursor;
			selection.data[ i ].cursor = selection.data[ i ].anchor;
			selection.data[ i ].anchor = tmp;
		}
	} else if( key == 'g' ){
		for( size_t i = 0; i < selection.count; i++ ){
			selection.data[ i ].cursor = StringSelectLineNumber( &file, command_count );
			selection.data[ i ].anchor = selection.data[ i ].cursor;
		}
		command_count = 0;
	} else if( key == 'G' ){
		for( size_t i = 0; i < selection.count; i++ ){
			selection.data[ i ].cursor = 0;
			selection.data[ i ].anchor = file.len - 1;
		}
		command_count = 0;
	} else if( key == 'h' ){
		ProsessSelectionMove( StringSelectCharPrev );
	} else if( key == 'l' ){
		ProsessSelectionMove( StringSelectCharNext );
	} else if( key == 'b' ){
		ProsessSelectionMove( StringSelectWordPrev );
	} else if( key == 'e' ){
		ProsessSelectionMove( StringSelectWordNext );
	} else if( key == 'k' ){
		ProsessSelectionMove( StringSelectLinePrev );
	} else if( key == 'j' ){
		ProsessSelectionMove( StringSelectLineNext );
	} else if( key == 'm' ){
		ProsessSelectionMove( StringSelectParagraphPrev );
	} else if( key == 'n' ){
		ProsessSelectionMove( StringSelectParagraphNext );
	} else if( key == 'F' ){
		ProsessSelectionMoveChar( StringSelectFindCharPrev, GetInputBufferWait() );
	} else if( key == 'f' ){
		ProsessSelectionMoveChar( StringSelectFindCharNext, GetInputBufferWait() );
	} else if( key == 'z' ){
		ProsessSelectionMove( StringSelectLineStart );
	} else if( key == 'x' ){
		ProsessSelectionMove( StringSelectLineEnd );
	} else if( key == 'T' ){
		command_count = 0;
		for( size_t i = 0; i < selection.count; i++ ){
			if( !anchor_pinned ){
				selection.data[ i ].anchor = file.len - 1;
			}
			selection.data[ i ].cursor = file.len - 1;
		}
	} else if( key == 't' ){
		command_count = 0;
		for( size_t i = 0; i < selection.count; i++ ){
			if( !anchor_pinned ){
				selection.data[ i ].anchor = 0;
			}
			selection.data[ i ].cursor = 0;
		}
	} else if( key == 'y' ){
		command_count = 0;
		CopySelection();
	} else if( key == 'Y' ){
		command_count = 0;
		SelectCursorLine();
		CopySelection();
	} else if( key == 'd' ){
		command_count = 0;
		EditNew();
		CopySelection();
		DeleteSelection();
	} else if( key == 'D' ){
		command_count = 0;
		SelectCursorLine();
		EditNew();
		CopySelection();
		DeleteSelection();
	} else if( key == 'c' ){
		command_count = 0;
		EditNew();
		CopySelection();
		DeleteSelection();
		EditModeInit();
	} else if( key == 'C' ){
		command_count = 0;
		SelectCursorLine();
		EditNew();
		CopySelection();
		DeleteSelection();
		EditModeInit();
	} else if( key == 'p' ){
		command_count = 0;
		EditNew();
		PasteSelection();
	} else if( key == 'r' ){
		command_count = 0;
		EditNew();
		DeleteSelection();
		PasteSelection();
	} else if( key == 'R' ){
		command_count = 0;
		SelectCursorLine();
		EditNew();
		DeleteSelection();
		PasteSelection();
	} else if( key == '<' ){
		command_count = 0;
		ProsessSelectionMove( StringSelectLineStart );
		ProsessSelectionMove( StringSelectCharNext );
		EditNew();
		for( size_t i = 0; i < selection.count; i++ ){
			edit.data[ edit.undo_count - 1 ].data[ i ].index = selection.data[ i ].cursor;
		}
		ProsessEditDelete( 1 );
	} else if( key == '>' ){
		command_count = 0;
		ProsessSelectionMove( StringSelectLineStart );
		EditNew();
		for( size_t i = 0; i < selection.count; i++ ){
			edit.data[ edit.undo_count - 1 ].data[ i ].index = selection.data[ i ].cursor;
		}
		ProsessEditInsert( "\t", 1 );
	} else if( key == '(' ){
		command_count = 0;
		ProsessSelectionInside( selection.data[ i ].cursor, selection.data[ i ].anchor, '(', ')' );
	} else if( key == ')' ){
		command_count = 0;
		ProsessSelectionInside( selection.data[ i ].anchor, selection.data[ i ].cursor, '(', ')' );
	} else if( key == '[' ){
		command_count = 0;
		ProsessSelectionInside( selection.data[ i ].cursor, selection.data[ i ].anchor, '[', ']' );
	} else if( key == ']' ){
		command_count = 0;
		ProsessSelectionInside( selection.data[ i ].anchor, selection.data[ i ].cursor, '[', ']' );
	} else if( key == '{' ){
		command_count = 0;
		ProsessSelectionInside( selection.data[ i ].cursor, selection.data[ i ].anchor, '{', '}' );
	} else if( key == '}' ){
		command_count = 0;
		ProsessSelectionInside( selection.data[ i ].anchor, selection.data[ i ].cursor, '{', '}' );
	} else if( key == '\'' ){
		command_count = 0;
		ProsessSelectionInside( selection.data[ i ].anchor, selection.data[ i ].cursor, '\'', '\'' );
	} else if( key == '"' ){
		command_count = 0;
		ProsessSelectionInside( selection.data[ i ].anchor, selection.data[ i ].cursor, '"', '"' );
	} else if( key == ';' ){
		command_count = 0;
		for( size_t i = 1; selection.count > 1; ){
			SelectionFree( i );
		}
	} else if( key == ':' ){
		struct Selection tmp = selection.data[ 0 ];
		for( size_t i = 1; i < selection.count; i++ ){
			selection.data[ i - 1 ].cursor = selection.data[ i ].cursor;
			selection.data[ i - 1 ].anchor = selection.data[ i ].anchor;
			selection.data[ i - 1 ].clipboard.data = selection.data[ i ].clipboard.data;
			selection.data[ i - 1 ].clipboard.len = selection.data[ i ].clipboard.len;
			selection.data[ i - 1 ].clipboard.cap = selection.data[ i ].clipboard.cap;
		}
		selection.data[ selection.count - 1 ] = tmp;

	} else if( key == 's' ){
		command_count = 0;
		struct String search = { 0 };
		while( true ){
			char input_key = GetInputBufferWait();
			if( input_key == '\n' ){
				break;
			}
			if( input_key == ESCAPE_KEY ){
				StringFree( &search );
				break;
			}
			StringAppend( &search, &input_key, 1 );
		}
		if( search.len > 0 ){
			for( size_t i = 1; selection.count > 1; ){
				SelectionFree( i );
			}
			if( selection.data[ 0 ].cursor > selection.data[ 0 ].anchor ){
				size_t new_selection_index = StringSelectSubStringPrev( &file, selection.data[ 0 ].cursor, search.data, search.len );
				if( new_selection_index >= selection.data[ 0 ].anchor ){
					size_t selection_min = selection.data[ 0 ].anchor;
					selection.data[ 0 ].cursor = new_selection_index;
					selection.data[ 0 ].anchor = new_selection_index + search.len;
					while( true ){
						new_selection_index = StringSelectSubStringPrev( &file, new_selection_index, search.data, search.len );
						if( new_selection_index < selection_min ){
							break;
						}
						if( new_selection_index == 0 ){
							break;
						}
						SelectionNew( new_selection_index );
						selection.data[ selection.count - 1 ].anchor = new_selection_index + search.len;
					}
				}
			} else {
				size_t new_selection_index = StringSelectSubStringNext( &file, selection.data[ 0 ].cursor, search.data, search.len );
				if( new_selection_index <= selection.data[ 0 ].anchor ){
					size_t selection_max = selection.data[ 0 ].anchor;
					selection.data[ 0 ].cursor = new_selection_index;
					selection.data[ 0 ].anchor = new_selection_index + search.len;
					while( true ){
						new_selection_index = StringSelectSubStringNext( &file, new_selection_index, search.data, search.len );
						if( new_selection_index > selection_max ){
							break;
						}
						if( new_selection_index == file.len - 1 ){
							break;
						}
						SelectionNew( new_selection_index );
						selection.data[ selection.count - 1 ].anchor = new_selection_index + search.len;
					}
				}
			}
		}
		StringFree( &search );

	} else if( key == 'S' ){
		command_count = 0;
		for( size_t i = 1; selection.count > 1; ){
			SelectionFree( i );
		}
		if( selection.data[ 0 ].cursor > selection.data[ 0 ].anchor ){
			size_t new_selection_index = ( file.data[ selection.data[ 0 ].cursor ] == '\n' ) ?
				selection.data[ 0 ].cursor :
				StringSelectFindCharNext( &file, selection.data[ 0 ].cursor, '\n' );
			if( new_selection_index >= selection.data[ 0 ].anchor ){
				size_t selection_min = selection.data[ 0 ].anchor;
				selection.data[ 0 ].cursor = new_selection_index;
				selection.data[ 0 ].anchor = new_selection_index;
				while( true ){
					new_selection_index = StringSelectFindCharPrev( &file, new_selection_index, '\n' );
					if( new_selection_index < selection_min ){
						break;
					}
					if( new_selection_index == 0 ){
						if( file.data[ new_selection_index ] == '\n' ){
							SelectionNew( new_selection_index );
						}
						break;
					}
					SelectionNew( new_selection_index );
				}
			}
		} else {
			size_t new_selection_index = ( file.data[ selection.data[ 0 ].cursor ] == '\n' ) ?
				selection.data[ 0 ].cursor :
				StringSelectFindCharNext( &file, selection.data[ 0 ].cursor, '\n' );
			if( new_selection_index <= selection.data[ 0 ].anchor ){
				size_t selection_max = selection.data[ 0 ].anchor;
				selection.data[ 0 ].cursor = new_selection_index;
				selection.data[ 0 ].anchor = new_selection_index;
				while( true ){
					new_selection_index = StringSelectFindCharNext( &file, new_selection_index, '\n' );
					if( new_selection_index > selection_max ){
						break;
					}
					if( new_selection_index == file.len - 1 ){
						SelectionNew( new_selection_index );
						break;
					}
					SelectionNew( new_selection_index );
				}
			}
		}
	} else if( key >= '0' && key <= '9' ){
		command_count *= 10;
		command_count += key & 0xf;
	}
}

void ProsessInput(){
	char key = GetInputBufferRead();
	while( key != '\0' ){
		if( edit_mode == true ){
			if( key == ESCAPE_KEY ){
				edit_mode = false;
				key = GetInputBuffer();
			} else if( key == TAB_KEY || key == NEWLINE_KEY || ( key >= ' ' && key < DELETE_KEY )){
				char data[ INPUT_BUFFER_CAP ] = { 0 };
				size_t len = 0;
				do{
					data[ len ] = key;
					len++;
					key = GetInputBuffer();
				} while( key == TAB_KEY || key == NEWLINE_KEY || ( key >= ' ' && key < DELETE_KEY ));
				ProsessEditInsert( data, len );
			} else if( key == DELETE_KEY ){
				size_t len = 0;
				do{
					len++;
					key = GetInputBuffer();
				} while( key == DELETE_KEY );
				ProsessEditDelete( len );
			}
		} else {
			ProsessCommand( key );
			key = GetInputBuffer();
		}
	}
}

void ValidateSelection(){
	for( size_t i = 0; i < selection.count; i++ ){
		size_t selection_min = ( selection.data[ i ].cursor > selection.data[ i ].anchor ) ? selection.data[ i ].anchor : selection.data[ i ].cursor;
		size_t selection_max = ( selection.data[ i ].cursor > selection.data[ i ].anchor ) ? selection.data[ i ].cursor : selection.data[ i ].anchor;
		for( size_t j = selection.count - 1; j < selection.count; j-- ){
			if( i == j ){
				continue;
			}
			bool cursor_inside = ( selection.data[ j ].cursor >= selection_min && selection.data[ j ].cursor <= selection_max ) ? true : false;
			bool anchor_inside = ( selection.data[ j ].anchor >= selection_min && selection.data[ j ].anchor <= selection_max ) ? true : false;
			if( cursor_inside && anchor_inside ){
				SelectionFree( j );
			} else if( cursor_inside ){
				selection.data[ i ].anchor = selection.data[ j ].cursor;
			}
		}
	}
}

int main( int argc, char** argv ){
	LoadArgs(argc, argv);
	StringFromFile( &file, file_name );
	if( file.len == 0 || file.data[ file.len - 1 ] != '\n' ){
		StringAppend( &file, "\n", 1 );
	}
	EnableRawMode();
	SelectionNew( 0 );
	while( true ){
		LoadScreen();
		DrawScreen();
		ProsessInput();
		ValidateSelection();
	}
	return 0;
}
