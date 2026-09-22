#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "base.h"

#define error_mode 0
#define command_mode 1
#define edit_mode 2

typedef struct {
	i32 cursor;
	i32 anchor;
	i32 clipboard_count;
	char* clipboard;
} selection_struct;

typedef struct {
	i32 index;
	i32 inserted_count;
	i32 removed_count;
	char* inserted;
	char* removed;
} edit_struct;

typedef struct {
	u32 key;
	void (*function)( void );
} keybind;

void command_quit();
void command_move_char_next();
void command_move_char_prev();
void command_move_word_next();
void command_move_word_prev();
void command_move_line_next();
void command_move_line_prev();
void command_move_para_next();
void command_move_para_prev();
void command_move_line_end();
void command_move_line_start();
void command_move_file_end();
void command_move_file_start();
void command_swap_anchor_cursor();

#include "config.h"

struct termios cache_termios = { 0 };

char file_buffer[ max_file_size ] = { 0 };
i32 file_count = 0;

char frame_buffer[ max_frame_size ] = { 0 };
i32 frame_count = 0;

char input_buffer[ max_input_size ] = { 0 };
i32 input_count = 0;

char clipboard_buffer[ max_clipboard_size ] = { 0 };
selection_struct selection[ max_selection_count ] = { 0 };
i32 selection_count = 1;  // first selection is initalized to all zeros

char edit_buffer[ max_edit_size ] = { 0 };
edit_struct edit[ max_edit_count ] = { 0 };
i32 undo_count;
i32 redo_count;

char* file_name = NULL;

i32 screen_cols = 0;
i32 screen_rows = 0;

i8 mode = command_mode;

i32 string_line_start( char* source, i32 count, i32 index ){
	assert( source != NULL );
	assert( count >= 0 );
	assert( index >= 0 );
	assert( count > index );
	while( index > 0 && source[ index - 1 ] != '\n' ){
		index -= 1;
	}
	return index;
}

i32 string_next_line( char* source, i32 count, i32 index ){
	assert( source != NULL );
	assert( count >= 0 );
	assert( index >= 0 );
	assert( count > index );
	while( index < count - 1 && source[ index ] != '\n' ){
		index += 1;
	}
	if( index < count - 1 ){
		index += 1;
	}
	return index;
}

void buffer_append( char* buffer, i32* buffer_count, i32 buffer_max, char* src, i32 src_count ){
	assert( buffer != NULL );
	assert( src != NULL );
	assert( buffer_count != NULL );
	assert( buffer_max >= *buffer_count );
	assert( buffer_max >= *buffer_count + src_count );
	memmove( &buffer[ *buffer_count ], src, src_count );
	*buffer_count += src_count;
}

void draw_bar(){
	buffer_append( frame_buffer, &frame_count, max_frame_size, "BAR", 3  );
}

void draw_line_selection_start( i32 line_index, i8* cursor_end ){
	for( i32 i = 0; i < selection_count; i += 1 ){
		if( selection[ i ].anchor == line_index ){
			if( selection[ i ].anchor < selection[ i ].cursor ){
				if( i == 0 ){
					buffer_append( frame_buffer, &frame_count, max_frame_size, primary_selection_highlight_start, strlen( primary_selection_highlight_start ));
				} else {
					buffer_append( frame_buffer, &frame_count, max_frame_size, selection_highlight_start, strlen( selection_highlight_start ));
				}
			} else if( selection[ i ].anchor > selection[ i ].cursor ){
				if( i == 0 ){
					buffer_append( frame_buffer, &frame_count, max_frame_size, primary_selection_highlight_end, strlen( primary_selection_highlight_end ));
				} else {
					buffer_append( frame_buffer, &frame_count, max_frame_size, selection_highlight_end, strlen( selection_highlight_end ));
				}
			}
		}
		if( selection[ i ].cursor == line_index ){
			if( selection[ i ].cursor < selection[ i ].anchor ){
				if( i == 0 ){
					buffer_append( frame_buffer, &frame_count, max_frame_size, primary_selection_highlight_start, strlen( primary_selection_highlight_start ));
				} else {
					buffer_append( frame_buffer, &frame_count, max_frame_size, selection_highlight_start, strlen( selection_highlight_start ));
				}
			} else if( selection[ i ].cursor > selection[ i ].anchor ){
				if( i == 0 ){
					buffer_append( frame_buffer, &frame_count, max_frame_size, primary_selection_highlight_end, strlen( primary_selection_highlight_end ));
				} else {
					buffer_append( frame_buffer, &frame_count, max_frame_size, selection_highlight_end, strlen( selection_highlight_end ));
				}
			}
			if( i == 0 ){
				*cursor_end = 1;
				buffer_append( frame_buffer, &frame_count, max_frame_size, primary_cursor_highlight_start, strlen( primary_cursor_highlight_start ));
			} else {
				*cursor_end = 2;
				buffer_append( frame_buffer, &frame_count, max_frame_size, cursor_highlight_start, strlen( cursor_highlight_start ));
			}
		}
	}
}

void draw_line_selection_end( i8 cursor_end ){
	if( cursor_end == 1 ){
		buffer_append( frame_buffer, &frame_count, max_frame_size, primary_cursor_highlight_end, strlen( primary_cursor_highlight_end ));
	} else if( cursor_end == 2 ){
		buffer_append( frame_buffer, &frame_count, max_frame_size, cursor_highlight_end, strlen( cursor_highlight_end ));
	}
}

void draw_line( i32 line_index ){
	assert( line_index >= 0 );
	assert( line_index < max_frame_size );
	assert( line_index == 0 || file_buffer[ line_index - 1 ] == '\n' );
	i32 filled_cols = 0; 
	while( filled_cols < screen_cols ){
		i32 col_bytes = 0;
		i8 cursor_end = 0;
		draw_line_selection_start( line_index, &cursor_end );
		if(( file_buffer[ line_index ] == '\n' ) || ( file_buffer[ line_index ] == '\r' )){
			buffer_append( frame_buffer, &frame_count, max_frame_size, " ", 1 );
			draw_line_selection_end( cursor_end );
			break;
		} else if( file_buffer[ line_index ] == '\t' ){
			i32 tab_cols = tab_width - (( filled_cols + tab_width ) % tab_width );
			buffer_append( frame_buffer, &frame_count, max_frame_size, "                ", tab_cols );
			draw_line_selection_end( cursor_end );
			filled_cols += tab_cols;
			line_index += 1;
			continue;
		} else if(( file_buffer[ line_index ] & 0x80 ) == 0 ){  // ascii
			filled_cols += 1;
			col_bytes = 1;
		} else if(( file_buffer[ line_index ] & 0xe0 ) == 0xc0 ){  // two byte unicode
			filled_cols += 1;
			col_bytes = 2;
		} else if(( file_buffer[ line_index ] & 0xf0 ) == 0xe0 ){  // three byte unicode
			filled_cols += 1;
			col_bytes = 3;
		} else if(( file_buffer[ line_index ] & 0xf8 ) == 0xf0 ){  // four byte unicode
			filled_cols += 1;
			col_bytes = 4;
		} else {
			error( "Invalid utf8 encoding." );
		}
		buffer_append( frame_buffer, &frame_count, max_frame_size, &file_buffer[ line_index ], col_bytes );
		draw_line_selection_end( cursor_end );
		line_index += col_bytes;
	}
}

void draw_frame(){
	assert( frame_count == 0 );
	assert( selection_count > 0 );
	buffer_append( frame_buffer, &frame_count, max_frame_size, ansi_cursor_home ansi_reset_graphics ansi_erase_screen, strlen( ansi_cursor_home ansi_reset_graphics ansi_erase_screen ));
	i32 file_frame_index = string_line_start( file_buffer, file_count, selection[ 0 ].cursor );
	i32 preceding_empty_lines = 0;
	for( i32 i = 0; i < screen_rows / 2; i += 1 ){
		if( file_frame_index > 0 ){
			file_frame_index -= 1;
		} else {
			preceding_empty_lines = ( screen_rows / 2 ) - i;
			break;
		}
		file_frame_index = string_line_start( file_buffer, file_count, file_frame_index );
	}
	draw_bar();
	{
		if( selection[ 0 ].anchor < file_frame_index ){
			buffer_append( frame_buffer, &frame_count, max_frame_size, primary_selection_highlight_start, strlen( primary_selection_highlight_start ));
		} else {
			i8 highlight = 0;
			for( i32 i = 1; i < selection_count; i += 1 ){
				if( selection[ i ].cursor < file_frame_index ){
					highlight = !highlight;
				}
				if( selection[ i ].anchor < file_frame_index ){
					highlight = !highlight;
				}
			}
			if( highlight ){
				buffer_append( frame_buffer, &frame_count, max_frame_size, selection_highlight_start, strlen( selection_highlight_start ));
			}
		}
	}
	for( i32 i = 1; i < screen_rows; i += 1 ){
		buffer_append( frame_buffer, &frame_count, max_frame_size, "\n", 1 );
		if( preceding_empty_lines > 0 || file_frame_index >= file_count ){
			buffer_append( frame_buffer, &frame_count, max_frame_size, "~", 1 );
			preceding_empty_lines -= 1;
		} else {
			draw_line( file_frame_index );
			while( file_frame_index < file_count - 1 && file_buffer[ file_frame_index ] != '\n' ){
				file_frame_index += 1;
			}
			file_frame_index += 1;
		}
	}
	write( STDOUT_FILENO, frame_buffer, frame_count );
	frame_count = 0;
}

i32 utf8_ansi_length( char* src ){
	if( *src == 0x1b /* escape */ ){  // possible ansi escape sequence
		if( *src + 1 == '[' ){
			if( *src + 2 == 'A' ){  // up arrow
				return 3;
			}
		}
	}
	if(( *src & 0x80 ) == 0 ){  // ascii
		return 1;
	} else if(( *src & 0xe0 ) == 0xc0 ){  // two byte unicode
		return 2;
	} else if(( *src & 0xf0 ) == 0xe0 ){  // three byte unicode
		return 3;
	} else if(( *src & 0xf8 ) == 0xf0 ){  // four byte unicode
		return 4;
	} else {
		error( "Invalid Encoding" );
		return 0;
	}
}

u32 full_utf8_ansi_key( char* buffer, i32* index, i32 cap ){
	assert( buffer != NULL );
	i32 key_bytes = utf8_ansi_length( &buffer[ *index ]);
	assert( key_bytes > 0 );
	assert( *index + key_bytes < cap );
	u32 key = 0;
	for( i32 i = 0; i < key_bytes; i += 1 ){
		key <<= 8;
		key |= buffer[ *index + i ];
	}
	*index += key_bytes;
	return key;
}

void command_quit(){
	exit( 1 );
}

void command_move_char_next(){
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].anchor = selection[ i ].cursor;
		if( selection[ i ].cursor < file_count - 1 ){
			selection[ i ].cursor += utf8_ansi_length( &file_buffer[ selection[ i ].cursor ]);
		}
	}
}

void command_move_char_prev(){
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].anchor = selection[ i ].cursor;
		do {
			if( selection[ i ].cursor > 0 ){
				selection[ i ].cursor -= 1;
			} else {
				break;
			}
		} while(( file_buffer[ selection[ i ].cursor ] & 0xc0 ) == 0x80 ); // while is utf8_continuation_byte
	}
}

i8 is_word_whitespace( char c ){
	return ( c == ' ' ) || ( c == '\t' );
}

void command_move_word_next(){
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].anchor = selection[ i ].cursor;
		if(( selection[ i ].cursor < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor ] == '\n' )){
			selection[ i ].cursor += 1;
			break;
		}
		while(( selection[ i ].cursor < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor ] != '\n' ) && !is_word_whitespace( file_buffer[ selection[ i ].cursor ])){
			selection[ i ].cursor += 1;
		}
		while(( selection[ i ].cursor < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor ] != '\n' ) && is_word_whitespace( file_buffer[ selection[ i ].cursor ])){
			selection[ i ].cursor += 1;
		}
	}
}

void command_move_word_prev(){
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].anchor = selection[ i ].cursor;
		if(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] == '\n' )){
			selection[ i ].cursor -= 1;
			break;
		}
		while(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] != '\n' ) && is_word_whitespace( file_buffer[ selection[ i ].cursor - 1 ])){
			selection[ i ].cursor -= 1;
		}
		while(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] != '\n' ) && !is_word_whitespace( file_buffer[ selection[ i ].cursor - 1 ])){
			selection[ i ].cursor -= 1;
		}
	}
}

void command_move_line_next(){
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].anchor = selection[ i ].cursor;
		while(( selection[ i ].cursor < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor ] != '\n' )){
			selection[ i ].cursor += 1;
		}
		if( selection[ i ].cursor < file_count - 1 ){
			selection[ i ].cursor += 1;
		}
	}
}

void command_move_line_prev(){
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].anchor = selection[ i ].cursor;
		while(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] != '\n' )){
			selection[ i ].cursor -= 1;
		}
		if( selection[ i ].cursor > 0 ){
			selection[ i ].cursor -= 1;
		}
		while(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] != '\n' )){
			selection[ i ].cursor -= 1;
		}
	}
}

void command_move_para_next(){
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].anchor = selection[ i ].cursor;
		while( selection[ i ].cursor < file_count - 1 ){
			if(( file_buffer[ selection[ i ].cursor ] == '\n' ) && ( selection[ i ].cursor + 1 < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor + 1 ] == '\n' )){
				while(( selection[ i ].cursor < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor ] == '\n' )){
					selection[ i ].cursor += 1;
				}
				break;
			}
			selection[ i ].cursor += 1;
		}
	}
}

void command_move_para_prev(){
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].anchor = selection[ i ].cursor;
		while(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] == '\n' )){
			selection[ i ].cursor -= 1;
		}
		while( selection[ i ].cursor > 0 ){
			if(( file_buffer[ selection[ i ].cursor - 1 ] == '\n' ) && ( selection[ i ].cursor - 1 > 0 ) && ( file_buffer[ selection[ i ].cursor - 2 ] == '\n' )){
				break;
			}
			selection[ i ].cursor -= 1;
		}
	}
}

void command_move_line_end(){
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].anchor = selection[ i ].cursor;
		while(( selection[ i ].cursor < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor ] != '\n' )){
			selection[ i ].cursor += 1;
		}
	}
}

void command_move_line_start(){
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].anchor = selection[ i ].cursor;
		while(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] != '\n' )){
			selection[ i ].cursor -= 1;
		}
	}
}

void command_move_file_end(){
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].anchor = selection[ i ].cursor;
		selection[ i ].cursor = file_count - 1;
	}
}

void command_move_file_start(){
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].anchor = selection[ i ].cursor;
		selection[ i ].cursor = 0;
	}
}

void command_swap_anchor_cursor(){
	for( i32 i = 0; i < selection_count; i += 1 ){
		i32 tmp = selection[ i ].anchor;
		selection[ i ].anchor = selection[ i ].cursor;
		selection[ i ].cursor = tmp;
	}
}

void process_command( u32 key ){
	for( i32 i = 0; i < (i32) ( sizeof( command ) / sizeof( keybind )); i += 1 ){
		if( key == command[ i ].key ){
			command[ i ].function();
			break;
		}
	}
}

void process_input(){
	i32 input_index = 0;
	input_count = read( STDIN_FILENO, &input_buffer, max_input_size );
	assert( input_count > 0 );
	while( input_index < input_count ){
		u32 key = full_utf8_ansi_key( input_buffer, &input_index, max_input_size );
		if( mode == command_mode ){
			process_command( key );
		} else if( mode == edit_mode ){
			if( key == command_mode_key ){
				mode = command_mode;
			} else if( key == '\b' ){
			} else {
//				process_insert( key );
			}
		}
	}
	input_count = 0;
}

void open_file(){
	if( access( file_name, F_OK ) == 0 ){
		i32 fd = open( file_name, O_RDWR | O_CREAT );
		if( fd < 0 ){
			error( "The file '%s' could not be opened." );
		}
		struct stat sb;
		if( fstat( fd, &sb ) < 0 ){
			error( "Could not get file '%s' type." );
		}
		if(( sb.st_mode & S_IFMT) == S_IFREG) {
			file_count = read( fd, file_buffer, max_file_size );
			if( file_count < 0 ){
				error( "The file '%s' could not be read.", file_name );
			}
			if( file_count == max_file_size ){
				error( "The file '%s' is larger then 'max_file_size'.", file_name ); 
			}
		} else {
			error( "'%s' is not a regular file.", file_name );
		}
		close( fd );
	} else {
		// The file does not exist. Nothing needs to be done, lute starts with an empty file.
	}
}

void disable_raw_mode(){
        i32 failed = tcsetattr( STDIN_FILENO, TCSAFLUSH, &cache_termios );
        if( failed == -1 ){
		error( "This terminal is not supported. (Unable to exit raw mode (Sorry the terminal looks like this now))" );
        }
        write( STDOUT_FILENO, ansi_end_alt_screen ansi_cursor_show, strlen( ansi_end_alt_screen ansi_cursor_show ));
}

void enable_raw_mode(){
        i32 failed = tcgetattr( STDIN_FILENO, &cache_termios );
        if( failed == -1 ){
		error( "This terminal is not supported. (Unable to enter raw mode)" );
        }
        struct termios raw_termios = cache_termios;
//        raw_termios.c_oflag &= ~OPOST; // turns off /n into /r/n
        raw_termios.c_iflag &= ~( IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON );
        raw_termios.c_lflag &= ~( ECHO | ECHONL | ICANON | ISIG | IEXTEN );
        raw_termios.c_cflag &= ~( CSIZE | PARENB );
        raw_termios.c_cflag |= CS8;
//	raw_termios.c_cc[ VMIN ] = 0;
        failed = tcsetattr( STDIN_FILENO, TCSAFLUSH, &raw_termios );
        if( failed == -1 ){
		error( "This terminal is not supported. (Unable to enter raw mode)" );
        }
        write( STDOUT_FILENO, ansi_start_alt_screen ansi_cursor_hidden, strlen( ansi_start_alt_screen ansi_cursor_hidden ));
        atexit( disable_raw_mode );
}

void get_window_size(){
        struct winsize ws;
        i32 failed = ioctl( STDOUT_FILENO, TIOCGWINSZ, &ws );
        if( failed == -1 ){
		error( "This terminal is not supported. (Unable to get terminal window size)" );
        }
        screen_cols = ws.ws_col;
        screen_rows = ws.ws_row;
}

i32 main( i32 argc, char* argv[] ){
	if( argc != 2 ){
		error( "Usage: lute <filename>" );
	}
	file_name = argv[ 1 ];
	open_file();
	enable_raw_mode();
	get_window_size();
	draw_frame();
	while( 1 ){
		process_input();
		get_window_size();
		draw_frame();
	}
	sleep( 1 );
}
