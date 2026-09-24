#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

#define ansi_cursor_home "\x1b[H"
#define ansi_erase_screen "\x1b[2J"
#define ansi_erase_line "\x1b[2K"
#define ansi_erase_line_after_cursor "\x1b[0K"

#define ansi_cursor_show "\x1b[?25h"
#define ansi_cursor_hidden "\x1b[?25l"
#define ansi_start_alt_screen "\x1b[?1049h"
#define ansi_end_alt_screen "\x1b[?1049l"

#define ansi_reset_graphics "\x1b[0m"
#define ansi_bold_start "\x1b[1m"
#define ansi_bold_end "\x1b[22m"
#define ansi_underline_start "\x1b[4m"
#define ansi_underline_end "\x1b[24m"
#define ansi_inverse_start "\x1b[7m"
#define ansi_inverse_end "\x1b[27m"

#define ansi_background_black "\x1b[40m"
#define ansi_foreground_black "\x1b[30m"
#define ansi_foreground_red "\x1b[31m"
#define ansi_background_red "\x1b[41m"
#define ansi_foreground_green "\x1b[32m"
#define ansi_background_green "\x1b[42m"
#define ansi_foreground_yellow "\x1b[33m"
#define ansi_background_yellow "\x1b[43m"
#define ansi_foreground_blue "\x1b[34m"
#define ansi_background_blue "\x1b[44m"
#define ansi_foreground_magenta "\x1b[35m"
#define ansi_background_magenta "\x1b[45m"
#define ansi_foreground_cyan "\x1b[36m"
#define ansi_background_cyan "\x1b[46m"
#define ansi_foreground_white "\x1b[37m"
#define ansi_background_white "\x1b[47m"
#define ansi_foreground_default "\x1b[39m"
#define ansi_background_default "\x1b[49m"

#define error_mode 0
#define command_mode 1
#define edit_mode 2

typedef struct {
	i32 cursor;
	i32 anchor;
	i32 clipboard_count;
} selection_struct;

typedef struct {
	i32 index;
	i32 insert_count;
	i32 delete_count;
	i32 selection_number;
} edit;

typedef struct {
	char* key;
	void (*function)( void );
} keybind;

typedef struct {
	void (*function)( void );
} bar_item;

void bar_file_name();
void bar_mode();
void bar_selection();
void bar_line_number();
void bar_line_depth();

void command_quit();
void command_edit_mode();
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
i32 clipboard_count = 0;
selection_struct selection[ max_selection_count ] = { 0 };
i32 selection_count = 1;  // first selection is initalized to all zeros
i32 primary_selection_index = 0;

char edit_buffer[ max_edit_size ] = { 0 };
i32 edit_count = 0;
edit history[ max_edit_count ] = { 0 };
i32 undo_count = 0;
i32 redo_count = 0;

char* file_name = NULL;

i32 screen_cols = 0;
i32 screen_rows = 0;

i8 mode = command_mode;

i8 file_modified = 0;

void disable_raw_mode(){
	tcsetattr( STDIN_FILENO, TCSAFLUSH, &cache_termios );
	write( STDOUT_FILENO, ansi_end_alt_screen ansi_cursor_show, strlen( ansi_end_alt_screen ansi_cursor_show ));
}

void assert_failed( char* file, i32 line, const char* func, char* expression ){
	disable_raw_mode();
	fprintf( stderr, "%s%s:%d:%s%s \"%s\"\n", ansi_foreground_red, file, line, func, ansi_foreground_default, expression );
	fflush( stderr );
	exit( 1 );
}

#define assert( expression ){\
	if( !( expression )){\
		assert_failed( __FILE__, __LINE__, __func__, #expression );\
	}\
}

void error( char* format, ... ){
	disable_raw_mode();
	fprintf( stderr, "%sError%s ", ansi_foreground_red, ansi_foreground_default );
	{
		va_list args;
		va_start( args, format );
		vfprintf( stderr, format, args );
		va_end( args );
	}
	fprintf( stderr, "\n" );
	fflush( stderr );
	exit( 1 );
}

i32 utf8_ansi_next_length( char* src ){
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

i32 utf8_next_length( char* src ){
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

i32 utf8_prev_length( char* src ){
	i32 length = 1;
	src -= 1;
	while(( *src & 0xc0 ) == 0x80 ){  // while is utf8 continuation byte
		length += 1;
		src -= 1;
	}
	assert( length < 5 );
	return length;
}

i32 line_number( char* buffer, i32 buffer_count, i32 index ){
	assert( buffer != NULL );
	assert( buffer_count >= 0 );
	assert( index >= 0 );
	assert( buffer_count > index );
	i32 line = 1;
	for( i32 i = 0; i < index; i += 1 ){
		if( buffer[ i ] == '\n' ){
			line += 1;
		}
	}
	return line;
}

i32 line_depth( char* buffer, i32 buffer_count, i32 index ){
	assert( buffer != NULL );
	assert( buffer_count >= 0 );
	assert( index >= 0 );
	assert( buffer_count > index );
	i32 depth = 1;
	while( index > 0 && buffer[ index - 1 ] != '\n' ){
		index -= utf8_prev_length( &buffer[ index ] );
		depth += 1;
	}
	return depth;
}

i32 line_length( char* buffer, i32 buffer_count, i32 index ){
	assert( buffer != NULL );
	assert( buffer_count >= 0 );
	assert( index >= 0 );
	assert( buffer_count > index );
	while(( index > 0 ) && ( buffer[ index - 1 ] != '\n' )){
		index -= 1;
	}
	i32 length = 1;
	while(( index < file_count - 1 ) && ( buffer[ index ] != '\n' )){
		index += utf8_next_length( &buffer[ index ] );
		length += 1;
	}
	return length;
}

void buffer_append( char* dst, i32* dst_count, char* src, i32 src_count ){
	assert( dst != NULL );
	assert( *dst_count >= 0 );
	assert( src != NULL );
	assert( src_count >= 0 );
	memmove( &dst[ *dst_count ], src, src_count );
	*dst_count += src_count;
}

void buffer_insert( char* dst, i32* dst_count, i32 index, char* src, i32 src_count ){
	assert( dst != NULL );
	assert( *dst_count >= 0 );
	assert( index >= 0 );
	assert( src != NULL );
	assert( src_count >= 0 );
	memmove( &dst[ index + src_count ], &dst[ index ], *dst_count - index );
	memmove( &dst[ index ], src, src_count );
	*dst_count += src_count;
}

void buffer_delete( char* dst, i32* dst_count, i32 index, i32 count ){
	assert( dst != NULL );
	assert( *dst_count >= 0 );
	assert( index >= 0 );
	assert( count >= 0 );
	assert( index + count <= *dst_count );
	memmove( &dst[ index ], &dst[ index + count ], *dst_count - index - count );
	*dst_count -= count;
}

void frame_append( char* src, i32 src_count ){
	if( max_frame_size <= frame_count + src_count ){
		error( "Frame buffer overflow, increase max_frame_size" );
	}
	buffer_append( frame_buffer, &frame_count, src, src_count );
}

void bar_file_name(){
	frame_append( file_name, strlen( file_name ));
	if( file_modified ){
		frame_append( "*", 1 );
	}
	frame_append( "  ", 2 );
}

void bar_mode(){
	if( mode == command_mode ){
		frame_append( "Command  ", 9 );
	} else if( mode == edit_mode ){
		frame_append( "Edit  ", 6 );
	}
}

void bar_selection(){
	char buffer[ 128 ] = { 0 };
	i32 buffer_count = snprintf( buffer, 128, "%d/%d  ", primary_selection_index + 1, selection_count );
	frame_append( buffer, buffer_count );
}

void bar_line_number(){
	char buffer[ 128 ] = { 0 };
	i32 buffer_count = snprintf( buffer, 128, "%d/%d  ", line_number( file_buffer, file_count, selection[ primary_selection_index ].cursor ), line_number( file_buffer, file_count, file_count - 1 ));
	frame_append( buffer, buffer_count );
}

void bar_line_depth(){
	char buffer[ 128 ] = { 0 };
	i32 buffer_count = snprintf( buffer, 128, "%d/%d  ", line_depth( file_buffer, file_count, selection[ primary_selection_index ].cursor ), line_length( file_buffer, file_count, selection[ primary_selection_index ].cursor ));
	frame_append( buffer, buffer_count );
}

void draw_bar(){
	for( i32 i = 0; i < (i32)( sizeof( bar ) / sizeof( bar_item )); i += 1 ){
		bar[ i ].function();
	}
}

void draw_line_selection_start( i32 line_index, i8* cursor_end ){
	for( i32 i = 0; i < selection_count; i += 1 ){
		if( selection[ i ].anchor == line_index ){
			if( selection[ i ].anchor < selection[ i ].cursor ){
				if( i == primary_selection_index ){
					frame_append( primary_selection_highlight_start, strlen( primary_selection_highlight_start ));
				} else {
					frame_append( selection_highlight_start, strlen( selection_highlight_start ));
				}
			} else if( selection[ i ].anchor > selection[ i ].cursor ){
				if( i == primary_selection_index ){
					frame_append( primary_selection_highlight_end, strlen( primary_selection_highlight_end ));
				} else {
					frame_append( selection_highlight_end, strlen( selection_highlight_end ));
				}
			}
		}
		if( selection[ i ].cursor == line_index ){
			if( selection[ i ].cursor < selection[ i ].anchor ){
				if( i == primary_selection_index ){
					frame_append( primary_selection_highlight_start, strlen( primary_selection_highlight_start ));
				} else {
					frame_append( selection_highlight_start, strlen( selection_highlight_start ));
				}
			} else if( selection[ i ].cursor > selection[ i ].anchor ){
				if( i == primary_selection_index ){
					frame_append( primary_selection_highlight_end, strlen( primary_selection_highlight_end ));
				} else {
					frame_append( selection_highlight_end, strlen( selection_highlight_end ));
				}
			}
			if( i == primary_selection_index ){
				*cursor_end = 1;
				frame_append( primary_cursor_highlight_start, strlen( primary_cursor_highlight_start ));
			} else {
				*cursor_end = 2;
				frame_append( cursor_highlight_start, strlen( cursor_highlight_start ));
			}
		}
	}
}

void draw_line_selection_end( i8 cursor_end ){
	if( cursor_end == 1 ){
		frame_append( primary_cursor_highlight_end, strlen( primary_cursor_highlight_end ));
	} else if( cursor_end == 2 ){
		frame_append( cursor_highlight_end, strlen( cursor_highlight_end ));
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
			frame_append( " ", 1 );
			draw_line_selection_end( cursor_end );
			break;
		} else if( file_buffer[ line_index ] == '\t' ){
			i32 tab_cols = tab_width - (( filled_cols + tab_width ) % tab_width );
			frame_append( "                ", tab_cols );
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
		frame_append( &file_buffer[ line_index ], col_bytes );
		draw_line_selection_end( cursor_end );
		line_index += col_bytes;
	}
}

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

void draw_frame(){
	assert( frame_count == 0 );
	assert( selection_count > 0 );
	frame_append( ansi_cursor_home ansi_reset_graphics ansi_erase_screen, strlen( ansi_cursor_home ansi_reset_graphics ansi_erase_screen ));
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
			frame_append( primary_selection_highlight_start, strlen( primary_selection_highlight_start ));
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
				frame_append( selection_highlight_start, strlen( selection_highlight_start ));
			}
		}
	}
	for( i32 i = 1; i < screen_rows; i += 1 ){
		frame_append( "\n", 1 );
		if( preceding_empty_lines > 0 || file_frame_index >= file_count ){
			frame_append( "~", 1 );
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

void command_quit(){
	disable_raw_mode();
	exit( 1 );
}

void command_edit_mode(){
	mode = edit_mode;
	redo_count = 0;
	clipboard_count = 0;
	for( i32 i = 0; i < selection_count; i += 1 ){
		history[ undo_count ].insert_count = 0;
		history[ undo_count ].delete_count = 0;
		history[ undo_count ].selection_number = i;
		history[ undo_count ].index = selection[ i ].cursor;
		undo_count += 1;
		selection[ i ].clipboard_count = 0;
		selection[ i ].anchor = selection[ i ].cursor;
	}
}

void command_move_char_next(){
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].anchor = selection[ i ].cursor;
		if( selection[ i ].cursor < file_count - 1 ){
			selection[ i ].cursor += utf8_ansi_next_length( &file_buffer[ selection[ i ].cursor ]);
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
				continue;
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
			continue;
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
			continue;
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

void process_insert( char* insert, i32 insert_bytes ){
	assert( insert != NULL );
	assert( insert_bytes > 0 );
	file_modified = 1;
	i32 edit_index = 0;
	for( i32 i = 0; i < undo_count - selection_count; i += 1 ){
		edit_index += history[ i ].insert_count;
		edit_index += history[ i ].delete_count;
	}
	i32 clipboard_index = 0;
	for( i32 i = 0; i < selection_count; i += 1 ){
// edit history
		i32 history_index = undo_count - selection_count + i;
		if( max_edit_size <= edit_count + insert_bytes ){
			error( "Edit buffer overflow, increase max_edit_size" );
		}
		buffer_insert( edit_buffer, &edit_count, edit_index + history[ history_index ].insert_count, insert, insert_bytes );
		history[ history_index ].insert_count += insert_bytes;
		edit_index += history[ history_index ].insert_count;
		edit_index += history[ history_index ].delete_count;
// clipboard
		if( max_clipboard_size <= clipboard_count + insert_bytes ){
			error( "Clipboard buffer overflow, increase max_clipboard_size" );
		}
		buffer_insert( clipboard_buffer, &clipboard_count, clipboard_index + selection[ i ].clipboard_count, insert, insert_bytes );
		selection[ i ].clipboard_count += insert_bytes;
		clipboard_index += selection[ i ].clipboard_count;
// selection
		selection[ i ].cursor += i * insert_bytes;
		if( max_file_size <= file_count + insert_bytes ){
			error( "File buffer overflow, increase max_file_size" );
		}
		buffer_insert( file_buffer, &file_count, selection[ i ].cursor, insert, insert_bytes );
		selection[ i ].cursor += insert_bytes;
		selection[ i ].anchor = selection[ i ].cursor;
	}
}

void process_delete(){
	file_modified = 1;
	i32 total_deleted = 0;
	i32 edit_index = 0;
	for( i32 i = 0; i < undo_count - selection_count; i += 1 ){
		edit_index += history[ i ].insert_count;
		edit_index += history[ i ].delete_count;
	}
	i32 clipboard_index = 0;
	for( i32 i = 0; i < selection_count; i += 1 ){
		if( selection[ i ].cursor > 0 ){
			i32 delete_bytes = utf8_prev_length( &file_buffer[ selection[ i ].cursor - total_deleted ]); 
			total_deleted += delete_bytes;
// edit history
			i32 history_index = undo_count - selection_count + i;
			if( history[ history_index ].insert_count >= delete_bytes ){
				history[ history_index ].insert_count -= delete_bytes;
				buffer_delete( edit_buffer, &edit_count, edit_index + history[ history_index ].insert_count, delete_bytes );
				edit_index += history[ history_index ].insert_count;
				edit_index += history[ history_index ].delete_count;
			} else if( history[ history_index ].insert_count == 0 ){
				if( max_edit_size <= edit_count + delete_bytes ){
					error( "Edit buffer overflow, increase max_edit_size" );
				}
				buffer_insert( edit_buffer, &edit_count, edit_index, &file_buffer[ selection[ i ].cursor - total_deleted ], delete_bytes );
				history[ history_index ].delete_count += delete_bytes;
				edit_index += history[ history_index ].delete_count;
			} else {
				assert( 0 );
			}
// clipboard
			if( clipboard_count - delete_bytes >= 0 ){
				selection[ i ].clipboard_count -= delete_bytes;
				buffer_delete( clipboard_buffer, &clipboard_count, clipboard_index + selection[ i ].clipboard_count, delete_bytes );
			}
			clipboard_index += selection[ i ].clipboard_count;
// selection
			selection[ i ].cursor -= total_deleted;
			selection[ i ].anchor = selection[ i ].cursor;
			buffer_delete( file_buffer, &file_count, selection[ i ].cursor, delete_bytes );
		}
	}
}

void process_input(){
	i32 input_index = 0;
	input_count = read( STDIN_FILENO, &input_buffer, max_input_size );
	assert( input_count > 0 );
	while( input_index < input_count ){
		i32 key_bytes = utf8_ansi_next_length( &input_buffer[ input_index ]);
		assert( key_bytes > 0 );
		assert( input_index + key_bytes < max_input_size );
		if( mode == command_mode ){
			for( i32 i = 0; i < (i32)( sizeof( command ) / sizeof( keybind )); i += 1 ){
				if(( key_bytes == (i32) strlen( command[ i ].key )) && ( memcmp( &input_buffer[ input_index ], command[ i ].key, key_bytes ) == 0 )){
					command[ i ].function();
				}
			}
		} else if( mode == edit_mode ){
			if(( key_bytes == 1 ) && ( input_buffer[ input_index ] == '\x1b' )){
				mode = command_mode;
			} else if(( key_bytes != 1 ) && ( input_buffer[ input_index ] == '\x1b' )){  // trying to input ansi escape sequence
				// dont do anyting
			} else if(( key_bytes == 1 ) && (( input_buffer[ input_index ] == '\b' ) || ( input_buffer[ input_index ] == 0x7f /* delete */  ))){
				process_delete();
			} else {
				i32 insert_bytes = 0;
				while(( input_index + insert_bytes < input_count ) && ( input_buffer[ input_index + insert_bytes ] != '\b' ) && ( input_buffer[ input_index + insert_bytes ] != '\x1b' )){
					insert_bytes += 1;
				}
				process_insert( &input_buffer[ input_index ], insert_bytes );
				input_index += insert_bytes - key_bytes;
			}
		}
		input_index += key_bytes;
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

void enable_raw_mode(){
        i32 failed = tcgetattr( STDIN_FILENO, &cache_termios );
        if( failed == -1 ){
		error( "This terminal is not supported. (Unable to enter raw mode)" );
        }
        struct termios raw_termios = cache_termios;
//        raw_termios.c_oflag &= ~OPOST; // turns off /n into /r/n
        raw_termios.c_iflag &= ~( IGNBRK | BRKINT | PARMRK | ISTRIP | /* INLCR | IGNCR | ICRNL | */ IXON );
        raw_termios.c_lflag &= ~( ECHO | ECHONL | ICANON | ISIG | IEXTEN );
        raw_termios.c_cflag &= ~( CSIZE | PARENB );
        raw_termios.c_cflag |= CS8;
//	raw_termios.c_cc[ VMIN ] = 0;
        failed = tcsetattr( STDIN_FILENO, TCSAFLUSH, &raw_termios );
        if( failed == -1 ){
		error( "This terminal is not supported. (Unable to enter raw mode)" );
        }
        write( STDOUT_FILENO, ansi_start_alt_screen ansi_cursor_hidden, strlen( ansi_start_alt_screen ansi_cursor_hidden ));
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
	disable_raw_mode();
}
