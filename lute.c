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

#include "ansi.h"

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

#define error_mode 0
#define command_mode 1
#define edit_mode 2
#define search_mode 3
#define find_next_mode 4
#define find_prev_mode 5
#define append_find_next_mode 6
#define append_find_prev_mode 7

typedef struct {
	i64 cursor;
	i64 anchor;
	i64 clipboard_count;
} selection_struct;

typedef struct {
	i64 index;
	i64 insert_count;
	i64 delete_count;
	i64 selection_index;
} edit;

typedef struct {
	char* key;
	void (*function)( void );
} keybind;

typedef struct {
	void (*function)( void );
} bar_item;

void bar_file_name();
void bar_warning();
void bar_mode();
void bar_selection();
void bar_line_number();
void bar_line_depth();
void bar_command_count();
void bar_search_string();

void command_quit();
void command_write();
void command_write_quit();
void command_edit_mode();
void command_edit_newline();
void command_indent();
void command_deindent();
void command_undo();
void command_redo();
void command_paste();
void command_copy();
void command_delete();
void command_change();
void command_replace();
void command_line_copy();
void command_line_delete();
void command_line_change();
void command_line_replace();
void command_split();
void command_split_newline();
void command_split_collapse();
void command_split_next();
void command_split_prev();
void command_move_char_next();
void command_move_char_prev();
void command_move_word_next();
void command_move_word_prev();
void command_move_line_next();
void command_move_line_prev();
void command_move_para_next();
void command_move_para_prev();
void command_move_find_next();
void command_move_find_prev();
void command_move_append_char_next();
void command_move_append_char_prev();
void command_move_append_word_next();
void command_move_append_word_prev();
void command_move_append_line_next();
void command_move_append_line_prev();
void command_move_append_para_next();
void command_move_append_para_prev();
void command_move_append_find_next();
void command_move_append_find_prev();
void command_move_line_end();
void command_move_line_start();
void command_move_file_end();
void command_move_file_start();
void command_select_inside_paren();
void command_select_inside_bracket();
void command_select_inside_curly();
void command_select_inside_double_quote();
void command_swap_anchor_cursor();
void command_select_entire_file();
void command_count_goto();
void command_count_1();
void command_count_2();
void command_count_3();
void command_count_4();
void command_count_5();
void command_count_6();
void command_count_7();
void command_count_8();
void command_count_9();
void command_count_0();

#include "config.h"

struct termios cache_termios = { 0 };
i8 raw_mode_enabled = 0;

char file_buffer[ max_file_bytes ] = { 0 };
i64 file_count = 0;

char frame_buffer[ max_frame_bytes ] = { 0 };
i64 frame_count = 0;

char input_buffer[ max_input_bytes ] = { 0 };
i64 input_count = 0;

char search_buffer[ max_search_bytes ] = { 0 };
i64 search_count = 0;

char clipboard_buffer[ max_clipboard_bytes ] = { 0 };
i64 clipboard_count = 0;
selection_struct selection[ max_selection_count ] = { 0 };
i64 selection_count = 1;  // first selection is initalized to all zeros
i64 primary_selection_index = 0;

char edit_buffer[ max_edit_bytes ] = { 0 };
i64 edit_count = 0;
edit history[ max_edit_count ] = { 0 };
i64 undo_count = 0;
i64 redo_count = 0;

char* file_name = NULL;

i32 screen_cols = 0;
i32 screen_rows = 0;

i8 mode = command_mode;
i64 command_count = 0;

i8 file_modified = 0;
char* warning = NULL;

void disable_raw_mode(){
	if( raw_mode_enabled ){
		tcsetattr( STDIN_FILENO, TCSAFLUSH, &cache_termios );
		write( STDOUT_FILENO, ansi_end_alt_screen ansi_cursor_show, strlen( ansi_end_alt_screen ansi_cursor_show ));
	}
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

i32 utf8_next_length( char* src ){
	assert( src != NULL );
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
	assert( src != NULL );
	i32 length = 1;
	src -= 1;
	while(( *src & 0xc0 ) == 0x80 ){  // while is utf8 continuation byte
		length += 1;
		src -= 1;
	}
	assert( length < 5 );
	return length;
}

i64 line_number( char* buffer, i64 buffer_count, i64 index ){
	assert( buffer != NULL );
	assert( buffer_count >= 0 );
	assert( index >= 0 );
	assert( buffer_count > index );
	i64 line = 1;
	for( i32 i = 0; i < index; i += 1 ){
		if( buffer[ i ] == '\n' ){
			line += 1;
		}
	}
	return line;
}

i64 line_depth( char* buffer, i64 buffer_count, i64 index ){
	assert( buffer != NULL );
	assert( buffer_count >= 0 );
	assert( index >= 0 );
	assert( buffer_count > index );
	i64 depth = 1;
	while( index > 0 && buffer[ index - 1 ] != '\n' ){
		index -= utf8_prev_length( &buffer[ index ] );
		depth += 1;
	}
	return depth;
}

i64 line_length( char* buffer, i64 buffer_count, i64 index ){
	assert( buffer != NULL );
	assert( buffer_count >= 0 );
	assert( index >= 0 );
	assert( buffer_count > index );
	while(( index > 0 ) && ( buffer[ index - 1 ] != '\n' )){
		index -= 1;
	}
	i64 length = 1;
	while(( index < file_count - 1 ) && ( buffer[ index ] != '\n' )){
		index += utf8_next_length( &buffer[ index ] );
		length += 1;
	}
	return length;
}

void buffer_append( char* dst, i64* dst_count, char* src, i64 src_count ){
	assert( dst != NULL );
	assert( *dst_count >= 0 );
	assert( src != NULL );
	assert( src_count >= 0 );
	memmove( &dst[ *dst_count ], src, src_count );
	*dst_count += src_count;
}

void buffer_insert( char* dst, i64* dst_count, i64 index, char* src, i64 src_count ){
	assert( dst != NULL );
	assert( *dst_count >= 0 );
	assert( index >= 0 );
	assert( src != NULL );
	assert( src_count >= 0 );
	memmove( &dst[ index + src_count ], &dst[ index ], *dst_count - index );
	memmove( &dst[ index ], src, src_count );
	*dst_count += src_count;
}

void buffer_delete( char* dst, i64* dst_count, i64 index, i64 count ){
	assert( dst != NULL );
	assert( *dst_count >= 0 );
	assert( index >= 0 );
	assert( count >= 0 );
	assert( index + count <= *dst_count );
	memmove( &dst[ index ], &dst[ index + count ], *dst_count - index - count );
	*dst_count -= count;
}

void frame_append( char* src, i64 src_count ){
	if( max_frame_bytes <= frame_count + src_count ){
		error( "Frame buffer overflow, increase max_frame_bytes" );
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

void bar_warning(){
	if( warning != NULL ){
		frame_append( ansi_background_red, strlen( ansi_background_red ));
		frame_append( warning, strlen( warning ));
		frame_append( ansi_background_default, strlen( ansi_background_default ));
		frame_append( "  ", 2 );
	}
}

void bar_mode(){
	if( mode == command_mode ){
		frame_append( "Command  ", 9 );
	} else if( mode == edit_mode ){
		frame_append( "Edit  ", 6 );
	} else if( mode == search_mode ){
		frame_append( "Search  ", 8 );
	} else if( mode == find_next_mode ){
		frame_append( "Find Next  ", 11 );
	} else if( mode == find_prev_mode ){
		frame_append( "Find Prev  ", 11 );
	} else if( mode == append_find_next_mode ){
		frame_append( "Append Find Next  ", 18 );
	} else if( mode == append_find_prev_mode ){
		frame_append( "Append Find Prev  ", 18 );
	}
}

void bar_selection(){
	char buffer[ 128 ] = { 0 };
	i64 buffer_count = snprintf( buffer, 128, "%ld/%ld  ", primary_selection_index + 1, selection_count );
	frame_append( buffer, buffer_count );
}

void bar_line_number(){
	char buffer[ 128 ] = { 0 };
	i64 buffer_count = snprintf( buffer, 128, "%ld/%ld  ", line_number( file_buffer, file_count, selection[ primary_selection_index ].cursor ), line_number( file_buffer, file_count, file_count - 1 ));
	frame_append( buffer, buffer_count );
}

void bar_line_depth(){
	char buffer[ 128 ] = { 0 };
	i64 buffer_count = snprintf( buffer, 128, "%ld/%ld  ", line_depth( file_buffer, file_count, selection[ primary_selection_index ].cursor ), line_length( file_buffer, file_count, selection[ primary_selection_index ].cursor ));
	frame_append( buffer, buffer_count );
}

void bar_command_count(){
	if( command_count > 0 ){
		char buffer[ 128 ] = { 0 };
		i64 buffer_count = snprintf( buffer, 128, "%ld  ", command_count );
		frame_append( buffer, buffer_count );
	}
}

void bar_search_string(){
	if( search_count > 0 ){
		char buffer[ 128 ] = { 0 };
		i64 buffer_count = snprintf( buffer, 128, "\"%.*s\"  ", (i32) search_count, search_buffer );
		frame_append( buffer, buffer_count );
	}
}

i64 selection_min( i64 index ){
	return (selection[ index ].cursor > selection[ index ].anchor) ? selection[ index ].anchor : selection[ index ].cursor;
}

i64 selection_max( i64 index ){
	return (selection[ index ].cursor > selection[ index ].anchor) ? selection[ index ].cursor : selection[ index ].anchor;
}

i64 selection_bytes( i64 index ){
	return (selection[ index ].cursor > selection[ index ].anchor) ? selection[ index ].cursor - selection[ index ].anchor : selection[ index ].anchor - selection[ index ].cursor;
}

i64 selection_length( i64 index ){
	i64 length = 0;
	for( i64 i = selection_min( index ); i < selection_max( index ); ){
		length += 1;
		i += utf8_next_length( &file_buffer[ i ]);
	}
	return length;
}

void new_undo(){
	redo_count = 0;
	for( i32 i = 0; i < selection_count; i += 1 ){
		if( max_edit_count <= undo_count + 1 ){
			error( "Edit count overflow, increase max_edit_count" );
		}
		history[ undo_count ].insert_count = 0;
		history[ undo_count ].delete_count = 0;
		history[ undo_count ].selection_index = i;
		history[ undo_count ].index = selection_min( i );
		undo_count += 1;
	}
}

void insert( char* insert, i64 insert_bytes ){
	assert( 0 );
}

void selection_delete(){
	file_modified = 1;
	i64 edit_index = 0;
	for( i32 i = 0; i < undo_count - selection_count; i += 1 ){
		edit_index += history[ i ].insert_count;
		edit_index += history[ i ].delete_count;
	}
	for( i32 i = 0; i < selection_count; i += 1 ){
		i64 history_index = undo_count - selection_count + i;
		assert( i == history[ history_index ].selection_index );
		edit_index += history[ history_index ].insert_count;
		if( max_edit_bytes <= edit_count + selection_bytes( i )){
			error( "Edit buffer overflow, increase max_edit_bytes" );
		}
		buffer_insert( edit_buffer, &edit_count, edit_index, &file_buffer[ selection_min( i )], selection_bytes( i ));
		history[ history_index ].delete_count += selection_bytes( i );
		edit_index += history[ history_index ].delete_count;
	}
	for( i32 i = selection_count - 1; i >= 0; i -= 1 ){
		for( i32 j = i + 1; j < selection_count; j += 1 ){
			selection[ j ].cursor -= selection_bytes( i );
			selection[ j ].anchor = selection[ j ].cursor;
		}
		buffer_delete( file_buffer, &file_count, selection_min( i ), selection_bytes( i ));
		selection[ i ].cursor = selection_min( i );
		selection[ i ].anchor = selection[ i ].cursor;
	}
}

void process_delete(){
	file_modified = 1;
	i64 total_deleted = 0;
	i64 edit_index = 0;
	for( i32 i = 0; i < undo_count - selection_count; i += 1 ){
		edit_index += history[ i ].insert_count;
		edit_index += history[ i ].delete_count;
	}
	i64 clipboard_index = 0;
	for( i32 i = 0; i < selection_count; i += 1 ){
		if( selection[ i ].cursor > 0 ){
			i64 delete_bytes = utf8_prev_length( &file_buffer[ selection[ i ].cursor - total_deleted ]); 
			total_deleted += delete_bytes;
// edit history
			i64 history_index = undo_count - selection_count + i;
			if( history[ history_index ].insert_count >= delete_bytes ){
				history[ history_index ].insert_count -= delete_bytes;
				buffer_delete( edit_buffer, &edit_count, edit_index + history[ history_index ].insert_count, delete_bytes );
				edit_index += history[ history_index ].insert_count;
				edit_index += history[ history_index ].delete_count;
			} else if( history[ history_index ].insert_count == 0 ){
				if( max_edit_bytes <= edit_count + delete_bytes ){
					error( "Edit buffer overflow, increase max_edit_bytes" );
				}
				buffer_insert( edit_buffer, &edit_count, edit_index, &file_buffer[ selection[ i ].cursor - total_deleted ], delete_bytes );
				history[ history_index ].delete_count += delete_bytes;
				history[ history_index ].index -= delete_bytes;
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

void selection_copy(){
	clipboard_count = 0;
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].clipboard_count = selection_bytes( i );
		if( max_clipboard_bytes <= clipboard_count + selection[ i ].clipboard_count ){
			error( "Clipboard buffer overflow, increase max_clipboard_bytes" );
		}
		buffer_append( clipboard_buffer, &clipboard_count, &file_buffer[ selection_min( i ) ], selection[ i ].clipboard_count );
	}
}

void selection_paste(){
	file_modified = 1;
	i64 edit_index = 0;
	for( i32 i = 0; i < undo_count - selection_count; i += 1 ){
		edit_index += history[ i ].insert_count;
		edit_index += history[ i ].delete_count;
	}
	i64 clipboard_index = 0;
	for( i32 i = 0; i < selection_count; i += 1 ){
// history
		i64 history_index = undo_count - selection_count + i;
		assert( i == history[ history_index ].selection_index );
		edit_index += history[ history_index ].insert_count;
		if( max_edit_bytes <= edit_count + selection[ i ].clipboard_count ){
			error( "Edit buffer overflow, increase max_edit_bytes" );
		}
		buffer_insert( edit_buffer, &edit_count, edit_index, &clipboard_buffer[ clipboard_index ], selection[ i ].clipboard_count );
		history[ history_index ].insert_count += selection[ i ].clipboard_count;
		edit_index += history[ history_index ].delete_count;
// selection
		selection[ i ].cursor += clipboard_index;
		selection[ i ].anchor = selection[ i ].cursor + selection[ i ].clipboard_count;
		if( max_file_bytes <= file_count + clipboard_index ){
			error( "File buffer overflow, increase max_file_bytes" );
		}
		buffer_insert( file_buffer, &file_count, selection[ i ].cursor, &clipboard_buffer[ clipboard_index ], selection[ i ].clipboard_count );
		clipboard_index += selection[ i ].clipboard_count;
	}
}

void select_cursor_line(){
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].anchor = selection[ i ].cursor + 1;
		while(( selection[ i ].anchor < file_count - 1 ) && ( file_buffer[ selection[ i ].anchor - 1 ] != '\n' )){
			selection[ i ].anchor += 1;
		}
		while(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] != '\n' )){
			selection[ i ].cursor -= 1;
		}
	}
}

void select_inside( char* left, i64 left_count, char* right, i64 right_count ){
	for( i32 i = 0; i < selection_count; i += 1 ){
		i64 min = selection[ i ].cursor;
		i64 max = selection[ i ].cursor;
		i64 min_nest = ( strncmp( &file_buffer[ min ], right, right_count ) == 0 ) ? 1 : 0;
		i64 max_nest = ( strncmp( &file_buffer[ max ], left, left_count ) == 0 ) ? 1 : 0;
		if(( selection[ i ].cursor > 0 ) && 
		   ( strncmp( &file_buffer[ selection[ i ].cursor - utf8_prev_length( &file_buffer[ selection[ i ].cursor ])], left, left_count ) == 0 ) && 
		   ( selection[ i ].anchor < file_count - 1 ) && 
		   ( strncmp( &file_buffer[ selection[ i ].anchor ], right, right_count ) == 0 )){
			min_nest += 1;
			max_nest += 1;
		}
		while( min > 0 ){
			min -= 1;
			if( strncmp( &file_buffer[ min ], left, left_count ) == 0 ){
				if( min_nest <= 0 ){
					min += 1;
					break;
				} else {
					min_nest -= 1;
				}
			}
			if( strncmp( &file_buffer[ min ], right, right_count ) == 0 ){
				min_nest += 1;
			}
		}
		while( max < file_count - 1 ){
			max += 1;
			if( strncmp( &file_buffer[ max ], right, right_count ) == 0 ){
				if( max_nest <= 0 ){
					break;
				} else {
					max_nest -= 1;
				}
			}
			if( strncmp( &file_buffer[ max ], left, left_count ) == 0 ){
				max_nest += 1;
			}
		}
		if( min > 0 && max < file_count - 1 ){
			selection[ i ].cursor = min;
			selection[ i ].anchor = max;
		}
	}
}

void selection_split( char* select, i64 select_bytes ){
	i64 min = selection_min( primary_selection_index );
	i64 max = selection_max( primary_selection_index );
	i64 new_selection_count = 0;
	while( min < max ){
		if( strncmp( &file_buffer[ min ], select, select_bytes) == 0 ){
			if( max_selection_count <= new_selection_count + 1 ){
				error( "Selection count overflow, increase max_selection_count" );
			}
			selection[ new_selection_count ].cursor = min;
			selection[ new_selection_count ].anchor = selection[ new_selection_count ].cursor + select_bytes;
			selection[ new_selection_count ].clipboard_count = 0;
			new_selection_count += 1;
		}
		min += 1;
	}
	if( new_selection_count > 0 ){
		selection_count = new_selection_count;
		primary_selection_index = 0;
		clipboard_count = 0;
	}
}

void select_next( char* select, i64 select_bytes ){
	assert( 0 );
}

void select_prev( char* select, i64 select_bytes ){
	assert( 0 );
}

void process_insert( char* insert, i64 insert_bytes ){
	assert( insert != NULL );
	assert( insert_bytes > 0 );
	file_modified = 1;
	i64 edit_index = 0;
	for( i32 i = 0; i < undo_count - selection_count; i += 1 ){
		edit_index += history[ i ].insert_count;
		edit_index += history[ i ].delete_count;
	}
	i64 clipboard_index = 0;
	for( i32 i = 0; i < selection_count; i += 1 ){
// edit history
		i64 history_index = undo_count - selection_count + i;
		if( max_edit_bytes <= edit_count + insert_bytes ){
			error( "Edit buffer overflow, increase max_edit_bytes" );
		}
		buffer_insert( edit_buffer, &edit_count, edit_index + history[ history_index ].insert_count, insert, insert_bytes );
		history[ history_index ].insert_count += insert_bytes;
		edit_index += history[ history_index ].insert_count;
		edit_index += history[ history_index ].delete_count;
// clipboard
		if( max_clipboard_bytes <= clipboard_count + insert_bytes ){
			error( "Clipboard buffer overflow, increase max_clipboard_bytes" );
		}
		buffer_insert( clipboard_buffer, &clipboard_count, clipboard_index + selection[ i ].clipboard_count, insert, insert_bytes );
		selection[ i ].clipboard_count += insert_bytes;
		clipboard_index += selection[ i ].clipboard_count;
// selection
		selection[ i ].cursor += i * insert_bytes;
		if( max_file_bytes <= file_count + insert_bytes ){
			error( "File buffer overflow, increase max_file_bytes" );
		}
		buffer_insert( file_buffer, &file_count, selection[ i ].cursor, insert, insert_bytes );
		selection[ i ].cursor += insert_bytes;
		selection[ i ].anchor = selection[ i ].cursor;
	}
}

void deoverlap_selections(){
	for( i32 i = 0; i < selection_count; i += 1 ){
		i64 selection_min = ( selection[ i ].cursor < selection[ i ].anchor ) ? selection[ i ].cursor : selection[ i ].anchor;
		i64 selection_max = ( selection[ i ].cursor > selection[ i ].anchor ) ? selection[ i ].cursor : selection[ i ].anchor;
		for( i32 j = selection_count - 1; j >= 0; j -= 1 ){
			if( i == j ){
				continue;
			}
			i8 j_cursor_inside = (( selection[ j ].cursor >= selection_min ) && ( selection[ j ].cursor <= selection_max )) ? 1 : 0;
			i8 j_anchor_inside = (( selection[ j ].anchor >= selection_min ) && ( selection[ j ].anchor <= selection_max )) ? 1 : 0;
			if( j_anchor_inside && j_cursor_inside ){  // cull j
				if( j < primary_selection_index ){
					primary_selection_index -= 1;
				} else if( j == primary_selection_index ){
					primary_selection_index = ( i < j ) ? i : j;
				}
				i64 clipboard_index = 0;
				for( i32 h = 0; h < j; h += 1 ){
					clipboard_index += selection[ h ].clipboard_count;
				}
				buffer_delete( clipboard_buffer, &clipboard_count, clipboard_index, selection[ j ].clipboard_count );
				for( i32 h = j; h < selection_count; h += 1 ){
					selection[ h ] = selection[ h + 1 ];
				}
				selection_count -= 1;
			} else if( j_anchor_inside ){
				selection[ j ].anchor = selection[ i ].cursor;
			}
		}
	}
}

void command_quit(){
	disable_raw_mode();
	exit( 1 );
}

void command_write(){
	i32 fd = open( file_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
	if( fd < 0 ){
		error( "The file '%s' could not be opened." );
	}
	i64 write_bytes = write( fd, file_buffer, file_count );
	if( write_bytes < 0 ){
		error( "The file '%s' could not be written.", file_name );
	}
	file_modified = 0;
	close( fd );
}

void command_write_quit(){
	command_write();
	command_quit();
}

void command_edit_mode(){
	mode = edit_mode;
	command_count = 0;
	clipboard_count = 0;
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].clipboard_count = 0;
		selection[ i ].anchor = selection[ i ].cursor;
	}
	new_undo();
}

void command_edit_newline(){
	command_move_line_end();
	deoverlap_selections();
	mode = edit_mode;
	command_count = 0;
	clipboard_count = 0;
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].clipboard_count = 0;
		selection[ i ].anchor = selection[ i ].cursor;
	}
	new_undo();
	process_insert( "\n", 1 );
}

void command_indent(){
	command_count = 0;
	command_move_line_start();
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].anchor = selection[ i ].cursor;
	}
	deoverlap_selections();
	new_undo();
	process_insert( tab_chars, strlen( tab_chars ));
	command_move_line_start();
}

void command_deindent(){
	command_count = 0;
	command_move_line_start();
	for( i32 i = 0; i < selection_count; i += 1 ){
		if( file_buffer[ selection[ i ].cursor ] == '\t' ){
			selection[ i ].anchor = selection[ i ].cursor + 1;
		} else {
			selection[ i ].anchor = selection[ i ].cursor;
		}
	}
	deoverlap_selections();
	new_undo();
	selection_delete();
}

void command_undo(){
	command_count = 0;
	if( undo_count == 0 ){
		return;
	}
	file_modified = 1;
	i64 new_selection_count = history[ undo_count - 1 ].selection_index + 1;
	undo_count -= new_selection_count;
	redo_count += new_selection_count;
	assert( undo_count >= 0 );
	i64 edit_index = 0;
	for( i32 i = 0; i < undo_count; i += 1 ){
		edit_index += history[ i ].insert_count;
		edit_index += history[ i ].delete_count;
	}
	for( i32 i = 0; i < new_selection_count; i += 1 ){
		i64 history_index = undo_count + i;
		buffer_delete( file_buffer, &file_count, history[ history_index ].index, history[ history_index ].insert_count );
		edit_index += history[ history_index ].insert_count;
		if( max_edit_bytes <= edit_count + history[ history_index ].delete_count ){
			error( "File buffer overflow, increase max_file_bytes" );
		}
		buffer_insert( file_buffer, &file_count, history[ history_index ].index, &edit_buffer[ edit_index ], history[ history_index ].delete_count );
		edit_index += history[ history_index ].delete_count;
		selection[ i ].cursor = history[ history_index ].index;
		selection[ i ].anchor = history[ history_index ].index + history[ history_index ].delete_count;
	}
	if( new_selection_count != selection_count ){
		for( i32 i = 0; i < selection_count; i += 1 ){
			selection[ i ].clipboard_count = 0;
		}
		selection_count = new_selection_count;
		clipboard_count = 0;
	}
}

void command_redo(){
	command_count = 0;
	if( redo_count == 0 ){
		return;
	}
	file_modified = 1;
	i64 new_selection_count = 0;
	while( new_selection_count == history[ undo_count + new_selection_count ].selection_index ){
		new_selection_count += 1;
	}
	i64 edit_index = 0;
	for( i32 i = 0; i < undo_count; i += 1 ){
		edit_index += history[ i ].insert_count;
		edit_index += history[ i ].delete_count;
	}
	i64 file_edit_offset = 0;
	for( i32 i = 0; i < new_selection_count; i += 1 ){
		i64 history_index = undo_count + i;
		buffer_delete( file_buffer, &file_count, history[ history_index ].index + file_edit_offset, history[ history_index ].delete_count );
		if( max_edit_bytes <= edit_count + history[ history_index ].delete_count ){
			error( "File buffer overflow, increase max_file_bytes" );
		}
		buffer_insert( file_buffer, &file_count, history[ history_index ].index + file_edit_offset, &edit_buffer[ edit_index ], history[ history_index ].insert_count );
		edit_index += history[ history_index ].insert_count;
		edit_index += history[ history_index ].delete_count;
		selection[ i ].cursor = history[ history_index ].index + file_edit_offset;
		selection[ i ].anchor = history[ history_index ].index + history[ history_index ].insert_count + file_edit_offset;
		file_edit_offset += history[ history_index ].insert_count;
		file_edit_offset -= history[ history_index ].delete_count;
	}
	undo_count += new_selection_count;
	redo_count -= new_selection_count;
	assert( redo_count >= 0 );
	if( new_selection_count != selection_count ){
		for( i32 i = 0; i < selection_count; i += 1 ){
			selection[ i ].clipboard_count = 0;
		}
		selection_count = new_selection_count;
		clipboard_count = 0;
	}
}

void command_paste(){
	command_count = 0;
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].anchor = selection[ i ].cursor;
	}
	new_undo();
	selection_paste();
}

void command_copy(){
	command_count = 0;
	selection_copy();
}

void command_delete(){
	command_count = 0;
	selection_copy();
	new_undo();
	selection_delete();
}

void command_change(){
	command_count = 0;
	selection_copy();
	new_undo();
	selection_delete();
	mode = edit_mode;
	clipboard_count = 0;
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].clipboard_count = 0;
		selection[ i ].anchor = selection[ i ].cursor;
	}
}

void command_replace(){
	command_count = 0;
	new_undo();
	selection_delete();
	selection_paste();
}

void command_line_copy(){
	select_cursor_line();
	command_copy();
}

void command_line_delete(){
	select_cursor_line();
	command_delete();
}

void command_line_change(){
	select_cursor_line();
	command_change();
}

void command_line_replace(){
	select_cursor_line();
	command_replace();
}

void command_split(){
	command_count = 0;
	mode = search_mode;
}

void command_split_newline(){
	command_count = 0;
	selection_split( "\n", 1 );
}

void command_split_collapse(){
	command_count = 0;
	selection[ 0 ] = selection[ primary_selection_index ];
	selection[ 0 ].clipboard_count = 0;
	primary_selection_index = 0;
	selection_count = 1;
	clipboard_count = 0;
}

void command_split_next(){
	do {
		if( primary_selection_index >= selection_count - 1 ){
			primary_selection_index = 0;
		} else {
			primary_selection_index += 1;
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_split_prev(){
	do {
		if( primary_selection_index <= 0 ){
			primary_selection_index = selection_count - 1;
		} else {
			primary_selection_index -= 1;
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_char_next(){
	do { 
		for( i32 i = 0; i < selection_count; i += 1 ){
			selection[ i ].anchor = selection[ i ].cursor;
			if( selection[ i ].cursor < file_count - 1 ){
				selection[ i ].cursor += utf8_next_length( &file_buffer[ selection[ i ].cursor ]);
			}
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_char_prev(){
	do {
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
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

i8 is_word_whitespace( char c ){
	return ( c == ' ' ) || ( c == '\t' );
}

void command_move_word_next(){
	do {
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
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_word_prev(){
	do {
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
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_line_next(){
	do {
		for( i32 i = 0; i < selection_count; i += 1 ){
			selection[ i ].anchor = selection[ i ].cursor;
			while(( selection[ i ].cursor < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor ] != '\n' )){
				selection[ i ].cursor += 1;
			}
			if( selection[ i ].cursor < file_count - 1 ){
				selection[ i ].cursor += 1;
			}
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_line_prev(){
	do {
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
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_para_next(){
	do {
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
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_para_prev(){
	do {
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
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_find_next(){
	mode = find_next_mode;
}

void command_move_find_prev(){
	mode = find_prev_mode;
}

void command_move_append_char_next(){
	do {
		for( i32 i = 0; i < selection_count; i += 1 ){
			if( selection[ i ].cursor < file_count - 1 ){
				selection[ i ].cursor += utf8_next_length( &file_buffer[ selection[ i ].cursor ]);
			}
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_append_char_prev(){
	do {
		for( i32 i = 0; i < selection_count; i += 1 ){
			do {
				if( selection[ i ].cursor > 0 ){
					selection[ i ].cursor -= 1;
				} else {
					continue;
				}
			} while(( file_buffer[ selection[ i ].cursor ] & 0xc0 ) == 0x80 ); // while is utf8_continuation_byte
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_append_word_next(){
	do {
		for( i32 i = 0; i < selection_count; i += 1 ){
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
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_append_word_prev(){
	do {
		for( i32 i = 0; i < selection_count; i += 1 ){
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
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_append_line_next(){
	do {
		for( i32 i = 0; i < selection_count; i += 1 ){
			while(( selection[ i ].cursor < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor ] != '\n' )){
				selection[ i ].cursor += 1;
			}
			if( selection[ i ].cursor < file_count - 1 ){
				selection[ i ].cursor += 1;
			}
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_append_line_prev(){
	do {
		for( i32 i = 0; i < selection_count; i += 1 ){
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
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_append_para_next(){
	do {
		for( i32 i = 0; i < selection_count; i += 1 ){
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
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_append_para_prev(){
	do {
		for( i32 i = 0; i < selection_count; i += 1 ){
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
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_append_find_next(){
	mode = append_find_next_mode;
}

void command_move_append_find_prev(){
	mode = append_find_prev_mode;
}

void command_move_line_end(){
	command_count = 0;
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].anchor = selection[ i ].cursor;
		while(( selection[ i ].cursor < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor ] != '\n' )){
			selection[ i ].cursor += 1;
		}
	}
}

void command_move_line_start(){
	command_count = 0;
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].anchor = selection[ i ].cursor;
		while(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] != '\n' )){
			selection[ i ].cursor -= 1;
		}
	}
}

void command_move_file_end(){
	command_count = 0;
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].anchor = selection[ i ].cursor;
		selection[ i ].cursor = file_count - 1;
	}
}

void command_move_file_start(){
	command_count = 0;
	for( i32 i = 0; i < selection_count; i += 1 ){
		selection[ i ].anchor = selection[ i ].cursor;
		selection[ i ].cursor = 0;
	}
}

void command_select_inside_paren(){
	do {
		select_inside( "(", 1, ")", 1 );
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_select_inside_bracket(){
	do {
		select_inside( "[", 1, "]", 1 );
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_select_inside_curly(){
	do {
		select_inside( "{", 1, "}", 1 );
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_select_inside_double_quote(){
	do {
		select_inside( "\"", 1, "\"", 1 );
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_swap_anchor_cursor(){
	command_count = 0;
	for( i32 i = 0; i < selection_count; i += 1 ){
		i32 tmp = selection[ i ].anchor;
		selection[ i ].anchor = selection[ i ].cursor;
		selection[ i ].cursor = tmp;
	}
}

void command_select_entire_file(){
	command_count = 0;
	selection[ 0 ].cursor = 0;
	selection[ 0 ].anchor = file_count - 1;
	selection[ 0 ].clipboard_count = 0;
	selection_count = 1;
	primary_selection_index = 0;
	clipboard_count = 0;
}

void command_count_goto(){
	i64 index = 0;
	while(( index < file_count - 1 ) && ( command_count > 1 )){
		if( file_buffer[ index - 1 ] == '\n' ){
			command_count -= 1;
		}
		index += 1;
	}
	command_count = 0;
	selection_count = 1;
	selection[ 0 ].cursor = index;
	selection[ 0 ].anchor = index;
}

void command_count_1(){
	command_count *= 10;
	command_count += 1;
}

void command_count_2(){
	command_count *= 10;
	command_count += 2;
}

void command_count_3(){
	command_count *= 10;
	command_count += 3;
}

void command_count_4(){
	command_count *= 10;
	command_count += 4;
}

void command_count_5(){
	command_count *= 10;
	command_count += 5;
}

void command_count_6(){
	command_count *= 10;
	command_count += 6;
}

void command_count_7(){
	command_count *= 10;
	command_count += 7;
}

void command_count_8(){
	command_count *= 10;
	command_count += 8;
}

void command_count_9(){
	command_count *= 10;
	command_count += 9;
}

void command_count_0(){
	command_count *= 10;
	command_count += 0;
}

i64 input_validate_next( char* buffer, i64 count, i64* index ){
/*
  Move foward one utf8 codepoint, or ansi escape sequence.
  If the codepoint is cut in half, return how many bytes of this codepoint are at the end of the buffer.
  This must handle any input that is valid in any part of the program.
  TODO: check for all possible invalid utf8.
  Currently does not throw out utf16 surrogates, overlong encodings, or codepoints above U+10ffff.
*/
	assert( buffer != NULL );
	assert( index != NULL );
	assert( count > *index );
	assert( count > 0 );
	assert( *index >= 0 );
	i64 clip = 0;
	if(( buffer[ *index ] & 0x80 ) == 0 ){  // ascii
		if( buffer[ *index ] == 0x1b /* escape */ ){
			if(( *index + 1 < count ) && ( buffer[ *index + 1 ] == '[' )){
				if(( *index + 2 < count ) && ( buffer[ *index + 2 ] == 'A' )){  // up arrow
					*index += 2;
				}
				if(( *index + 2 < count ) && ( buffer[ *index + 2 ] == 'B' )){  // down arrow
					*index += 2;
				}
				if(( *index + 2 < count ) && ( buffer[ *index + 2 ] == 'C' )){  // right arrow
					*index += 2;
				}
				if(( *index + 2 < count ) && ( buffer[ *index + 2 ] == 'D' )){  // left arrow
					*index += 2;
				}
			}
		}
		*index += 1;
	} else if(( buffer[ *index ] & 0xe0 ) == 0xc0 ){  // two byte unicode
		*index += 1;
		if( *index >= count ){
			clip = 1;
			goto end;
		}
		if(( buffer[ *index ] & 0xc0 ) != 0x80 ){
			error( "Input invalid utf8" );
		}
		*index += 1;
	} else if(( buffer[ *index ] & 0xf0 ) == 0xe0 ){  // three byte unicode
		*index += 1;
		if( *index >= count ){
			clip = 1;
			goto end;
		}
		if(( buffer[ *index ] & 0xc0 ) != 0x80 ){
			error( "Input invalid utf8" );
		}
		*index += 1;
		if( *index >= count ){
			clip = 2;
			goto end;
		}
		if(( buffer[ *index ] & 0xc0 ) != 0x80 ){
			error( "Input invalid utf8" );
		}
		*index += 1;
	} else if(( buffer[ *index ] & 0xf8 ) == 0xf0 ){  // four byte unicode
		*index += 1;
		if( *index >= count ){
			clip = 1;
			goto end;
		}
		if(( buffer[ *index ] & 0xc0 ) != 0x80 ){
			error( "Input invalid utf8" );
		}
		*index += 1;
		if( *index >= count ){
			clip = 2;
			goto end;
		}
		if(( buffer[ *index ] & 0xc0 ) != 0x80 ){
			error( "Input invalid utf8" );
		}
		*index += 1;
		if( *index >= count ){
			clip = 3;
			goto end;
		}
		if(( buffer[ *index ] & 0xc0 ) != 0x80 ){
			error( "Input invalid utf8" );
		}
		*index += 1;
	} else {
		error( "Input invalid utf8" );
	}
end:
	return clip;
}

i8 is_ascii_unprintable( char* buffer, i64 index, i64 bytes ){
	assert( buffer != NULL );
	assert( index >= 0 );
	assert( bytes > 0 );
	if(( bytes == 1 ) && (( buffer[ index ] >= 127 || buffer[ index ] < 32 )) && ( buffer[ index ] != '\n' ) && ( buffer[ index ] != '\t' )){
		return 1;
	}
	return 0;
}

i8 is_ansi_escape( char* buffer, i64 index, i64 bytes ){
	assert( buffer != NULL );
	assert( index >= 0 );
	assert( bytes > 0 );
	if(( bytes > 1 ) && ( buffer[ index ] == 0x1b )){
		return 1;
	}
	return 0;
}

i32 main( i32 argc, char* argv[] ){
	{
/*
  Set up the terminal state for the rest of the program.
  Enables most of 'raw mode' as defined by cfmakeraw in man 3 termios.
  Then start the alt screen and hide the cursor.
*/
	        i32 failed = tcgetattr( STDIN_FILENO, &cache_termios );
	        if( failed == -1 ){
			error( "This terminal is not supported. (Unable to enter raw mode)" );
	        }
	        struct termios raw_termios = cache_termios;
	        raw_termios.c_iflag &= ~( IGNBRK | BRKINT | PARMRK | ISTRIP | IXON );
	        raw_termios.c_lflag &= ~( ECHO | ECHONL | ICANON | ISIG | IEXTEN );
	        raw_termios.c_cflag &= ~( CSIZE | PARENB );
	        raw_termios.c_cflag |= CS8;
	        failed = tcsetattr( STDIN_FILENO, TCSAFLUSH, &raw_termios );
	        if( failed == -1 ){
			error( "This terminal is not supported. (Unable to enter raw mode)" );
	        }
	        write( STDOUT_FILENO, ansi_start_alt_screen ansi_cursor_hidden, strlen( ansi_start_alt_screen ansi_cursor_hidden ));
		raw_mode_enabled = 1;
	}
	if( argc != 2 ){
		error( "Usage: lute <filename>" );
	}
	file_name = argv[ 1 ];
	{
/*
  If the file can be opened and edited, then read the file into file_buffer.
  If the file does not exist, then create a new file, adding one '\n'.
  If the last byte is not '\n', then add it.
  Note: all lines must end in a '\n', and there must be at least one line.
  Check to make sure the file is valid utf8.
*/
		i64 file_index = 0;
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
				file_count = read( fd, file_buffer, max_file_bytes );
				if( file_count < 0 ){
					error( "The file '%s' could not be read.", file_name );
				}
				if( file_count == max_file_bytes ){
					error( "The file '%s' is larger then 'max_file_bytes'.", file_name );
				}
			} else {
				error( "'%s' is not a regular file.", file_name );
			}
			close( fd );
		}
		if(( file_count == 0 ) || ( file_buffer[ file_count - 1 ] != '\n' )){
			buffer_append( file_buffer, &file_count, "\n", 1 );
		}
		while( file_index < file_count ){
			i64 start_index = file_index;
			i64 clip = input_validate_next( file_buffer, file_count, &file_index );
			if( clip != 0 ){
				error( "File '%s' has an invalid or unsuported utf-8 encoding", file_name );
			}
			assert( start_index < file_index );
			if( is_ascii_unprintable( file_buffer, start_index, file_index - start_index )){
				error( "File '%s' has an invalid or unsuported utf-8 encoding", file_name );
			}
			if( is_ansi_escape( file_buffer, start_index, file_index - start_index )){
				error( "File '%s' has an invalid or unsuported utf-8 encoding", file_name );
			}
		}
	}
	while( 1 ){
		{
/*
  Get the dimensions of the terminal window.
  TODO: potentialy use the ansi escape sequences for position movement and reporting to remove the need for <sys/ioctl.h>.
*/
		        struct winsize ws;
		        i32 failed = ioctl( STDOUT_FILENO, TIOCGWINSZ, &ws );
		        if( failed == -1 ){
				error( "This terminal is not supported. (Unable to get terminal window size)" );
		        }
		        screen_cols = ws.ws_col;
		        screen_rows = ws.ws_row;
		}
		{
/*
  Draw the frame
*/
			assert( frame_count == 0 );
			assert( selection_count > 0 );
			frame_append( ansi_cursor_home ansi_reset_graphics ansi_erase_screen, strlen( ansi_cursor_home ansi_reset_graphics ansi_erase_screen ));
			i64 primary_line_number = line_number( file_buffer, file_count, selection[ primary_selection_index ].cursor );
			i64 draw_index = selection[ primary_selection_index ].cursor;
			i32 empty_lines_above = 0;
			{
/*
  Find where in the file to start drawing from and how many empty columns to draw before it.
*/
				while(( draw_index > 0 ) && ( file_buffer[ draw_index - 1 ] != '\n' )){
					draw_index -= 1;
				}
				for( i32 i = 0; i < screen_rows / 2; i += 1 ){
					if( draw_index == 0 ){
						empty_lines_above = ( screen_rows / 2 ) - i;
						break;
					}
					draw_index -= 1;
					while(( draw_index > 0 ) && ( file_buffer[ draw_index - 1 ] != '\n' )){
						draw_index -= 1;
					}
				}
			}
			if( bar_possition == top_bar ){
				for( i32 i = 0; i < (i32)( sizeof( bar ) / sizeof( bar_item )); i += 1 ){
					bar[ i ].function();
				}
				frame_append( "\n", 1 );
			}
			i8 highlight = 0;
			{
/*
  Find if the printed portion of the file starts in the middle a selection.
*/
				if( selection[ primary_selection_index ].anchor < draw_index ){
					highlight = 2;
				} else {
					for( i32 i = 0; i < selection_count; i += 1 ){
						if( selection[ i ].cursor < draw_index ){
							highlight = !highlight;
						}
						if( selection[ i ].anchor < draw_index ){
							highlight = !highlight;
						}
						if( selection[ i ].anchor >= draw_index || selection[ i ].cursor >= draw_index ){
							break;
						}
					}
				}
			}
			for( i32 i = 0; i < (( bar_possition == no_bar ) ? screen_rows : screen_rows - 1 ); i += 1 ){
				if( i != 0 ){
					frame_append( "\n", 1 );
				}
				if( empty_lines_above > 0 || draw_index >= file_count ){
					frame_append( "~", 1 );
					empty_lines_above -= 1;
				} else {
					i64 filled_cols = 0;
					if( draw_line_numbers == regular_line_numbers || draw_line_numbers == relitive_line_numbers ){
						filled_cols += 2;
						i64 line_print = 0;
						for( i64 i = primary_line_number + screen_rows / 2 - 1; i > 0; i /= 10 ){
							filled_cols += 1;
						}
						char buffer[ 128 ] = { '\0' };
						if( draw_line_numbers == regular_line_numbers ){
							line_print = primary_line_number - screen_rows / 2 + i;
						} else if( draw_line_numbers == relitive_line_numbers ){
							line_print = ( i == screen_rows / 2 ) ? primary_line_number : abs( screen_rows / 2 - i );
						}
						sprintf( buffer, " %*ld  ", (i32) filled_cols - 3, line_print );
						frame_append( buffer, filled_cols );
					}
					if( highlight == 1 ){
						frame_append( selection_highlight_start, strlen( selection_highlight_start ));
					} else if( highlight == 2 ){
						frame_append( primary_selection_highlight_start, strlen( primary_selection_highlight_start ));
					}
					while( filled_cols < screen_cols ){
						i32 col_bytes = 0;
						i8 cursor_end = 0;
						for( i32 i = 0; i < selection_count; i += 1 ){
/*
  If this column is the start or end of a highlight, insert that.
*/
							if( selection[ i ].anchor == draw_index ){
								if( selection[ i ].anchor < selection[ i ].cursor ){
									if( i == primary_selection_index ){
										highlight = 2;
										frame_append( primary_selection_highlight_start, strlen( primary_selection_highlight_start ));
									} else {
										highlight = 1;
										frame_append( selection_highlight_start, strlen( selection_highlight_start ));
									}
								} else if( selection[ i ].anchor > selection[ i ].cursor ){
									highlight = 0;
									if( i == primary_selection_index ){
										frame_append( primary_selection_highlight_end, strlen( primary_selection_highlight_end ));
									} else {
										frame_append( selection_highlight_end, strlen( selection_highlight_end ));
									}
								}
							}
							if( selection[ i ].cursor == draw_index ){
								if( selection[ i ].cursor < selection[ i ].anchor ){
									if( i == primary_selection_index ){
										highlight = 2;
										frame_append( primary_selection_highlight_start, strlen( primary_selection_highlight_start ));
									} else {
										highlight = 1;
										frame_append( selection_highlight_start, strlen( selection_highlight_start ));
									}
								} else if( selection[ i ].cursor > selection[ i ].anchor ){
									highlight = 0;
									if( i == primary_selection_index ){
										frame_append( primary_selection_highlight_end, strlen( primary_selection_highlight_end ));
									} else {
										frame_append( selection_highlight_end, strlen( selection_highlight_end ));
									}
								}
								if( i == primary_selection_index ){
									cursor_end = 2;
									frame_append( primary_cursor_highlight_start, strlen( primary_cursor_highlight_start ));
								} else {
									cursor_end = 1;
									frame_append( cursor_highlight_start, strlen( cursor_highlight_start ));
								}
							}
						}
						if(( file_buffer[ draw_index ] == '\n' ) || ( file_buffer[ draw_index ] == '\r' )){
							frame_append( " ", 1 );
							if( cursor_end == 2 ){
								frame_append( primary_cursor_highlight_end, strlen( primary_cursor_highlight_end ));
							} else if( cursor_end == 1 ){
								frame_append( cursor_highlight_end, strlen( cursor_highlight_end ));
							}
							break;
						} else if( file_buffer[ draw_index ] == '\t' ){
							i32 tab_cols = tab_width - (( filled_cols + tab_width ) % tab_width );
							frame_append( "                ", tab_cols );
							if( cursor_end == 2 ){
								frame_append( primary_cursor_highlight_end, strlen( primary_cursor_highlight_end ));
							} else if( cursor_end == 1 ){
								frame_append( cursor_highlight_end, strlen( cursor_highlight_end ));
							}
							filled_cols += tab_cols;
							draw_index += 1;
							continue;
						} else if(( file_buffer[ draw_index ] & 0x80 ) == 0 ){  // ascii
							filled_cols += 1;
							col_bytes = 1;
						} else if(( file_buffer[ draw_index ] & 0xe0 ) == 0xc0 ){  // two byte unicode
							filled_cols += 1;
							col_bytes = 2;
						} else if(( file_buffer[ draw_index ] & 0xf0 ) == 0xe0 ){  // three byte unicode
							filled_cols += 1;
							col_bytes = 3;
						} else if(( file_buffer[ draw_index ] & 0xf8 ) == 0xf0 ){  // four byte unicode
							filled_cols += 1;
							col_bytes = 4;
						} else {
							error( "Trying to print and invalid utf8 encoding." );
						}
						frame_append( &file_buffer[ draw_index ], col_bytes );
						if( cursor_end == 2 ){
							frame_append( primary_cursor_highlight_end, strlen( primary_cursor_highlight_end ));
						} else if( cursor_end == 1 ){
							frame_append( cursor_highlight_end, strlen( cursor_highlight_end ));
						}
						draw_index += col_bytes;
					}
					if( highlight == 1 ){
						frame_append( selection_highlight_end, strlen( selection_highlight_end ));
					} else if( highlight == 2 ){
						frame_append( primary_selection_highlight_end, strlen( primary_selection_highlight_end ));
					}
					while( draw_index < file_count - 1 && file_buffer[ draw_index ] != '\n' ){
						draw_index += 1;
					}
					draw_index += 1;
				}
			}
			if( bar_possition == bottom_bar ){
				frame_append( "\n", 1 );
				for( i32 i = 0; i < (i32)( sizeof( bar ) / sizeof( bar_item )); i += 1 ){
					bar[ i ].function();
				}
			}
			write( STDOUT_FILENO, frame_buffer, frame_count );
			frame_count = 0;
		}
		{
/*
  Wait for input then process it
*/
			i64 input_index = 0;
			i64 input_clip = 0;
			assert( input_count >= 0 );
			input_count = read( STDIN_FILENO, &input_buffer[ input_count ], max_input_bytes - input_count ) + input_count;  // will block until input
			assert( input_count > 0 );
			assert( input_count <= max_input_bytes );
			while( input_index < input_count ){
				if( mode == command_mode ){
					i64 start_index = input_index;
					input_clip = input_validate_next( input_buffer, input_count, &input_index );
					if( input_clip != 0 ){
						break;
					}
					assert( start_index < input_index );
					i64 key_bytes = input_index - start_index;
					assert( key_bytes > 0 );
					for( i64 i = 0; i < (i64)( sizeof( command ) / sizeof( keybind )); i += 1 ){
						if(( key_bytes == (i64) strlen( command[ i ].key )) && ( memcmp( &input_buffer[ start_index ], command[ i ].key, key_bytes ) == 0 )){
							command[ i ].function();
						}
					}
					if(( key_bytes == 1 ) && ( input_buffer[ start_index ] == 0x1b )){
						command_count = 0;
						warning = NULL;
					}
				} else if( mode == edit_mode ){
					i64 start_index = input_index;
					do {
						i64 loop_index = input_index;
						input_clip = input_validate_next( input_buffer, input_count, &input_index );
						if( input_clip != 0 ){
							break;
						}
						if( is_ansi_escape( input_buffer, loop_index, input_index - loop_index )){
							warning = "Can only input text";
							break;
						} else if( input_buffer[ loop_index ] == 0x1b /* escape */ ){
							mode = command_mode;
							break;
						} else if( input_buffer[ loop_index ] == 0x7f /* delete */ ){
							i64 insert_bytes = input_index - start_index - 1;
							if( insert_bytes > 0 ){
								process_insert( &input_buffer[ start_index ], insert_bytes );
							}
							process_delete();
							goto loop_end;
							break;
						} else if( is_ascii_unprintable( input_buffer, loop_index, input_index - loop_index )){
							warning = "Can only input text";
							break;
						}
					} while( input_index < input_count );
					i64 insert_bytes = input_index - start_index - input_clip;
					if( insert_bytes == 0 ){
						break;
					}
					process_insert( &input_buffer[ start_index ], insert_bytes );
					if( input_clip != 0 ){
						break;
					}
				} else if( mode == search_mode ){
					i64 start_index = input_index;
					input_clip = input_validate_next( input_buffer, input_count, &input_index );
					if( input_clip != 0 ){
						break;
					}
					assert( input_index > start_index );
					if( is_ansi_escape( input_buffer, start_index, input_index - start_index )){
						warning = "Can only input text";
					} else if( input_buffer[ start_index ] == 0x1b /* escape */ ){
						mode = command_mode;
						search_count = 0;
					} else if( input_buffer[ start_index ] == 0x7f /* delete */ ){
						if( search_count > 0 ){
							search_count -= 1;
						}
					} else if( input_buffer[ start_index ] == '\n' ){
						if( search_count > 0 ){
							selection_split( search_buffer, search_count );
						}
						mode = command_mode;
						search_count = 0;
					} else if( is_ascii_unprintable( input_buffer, start_index, input_index - start_index )){
						warning = "Can only input text";
					} else {
						for( i32 i = 0; i < input_index - start_index; i += 1 ){
							search_buffer[ search_count ] = input_buffer[ start_index + i ];
							search_count += 1;
						}
					}
				} else if(( mode == find_next_mode ) || ( mode == find_prev_mode ) || ( mode == append_find_next_mode ) || ( mode == append_find_prev_mode )){
					i64 start_index = input_index;
					input_clip = input_validate_next( input_buffer, input_count, &input_index );
					if( input_clip != 0 ){
						break;
					}
					assert( input_index > start_index );
					i64 key_bytes = input_index - start_index;
					if( is_ansi_escape( input_buffer, start_index, key_bytes )){
						warning = "Can only input text";
					} else if( input_buffer[ start_index ] == 0x1b /* escape */ ){
						// do nothing
					} else if( is_ascii_unprintable( input_buffer, start_index, key_bytes )){
						warning = "Can only input text";
					} else {
						do {
							for( i32 i = 0; i < selection_count; i += 1 ){
								if( mode == find_next_mode ){
									for( i64 j = selection[ i ].cursor + 1; j < file_count - 1; j += 1 ){
										if( strncmp( &file_buffer[ j ], &input_buffer[ start_index ], key_bytes ) == 0 ){
											selection[ i ].anchor = selection[ i ].cursor;
											selection[ i ].cursor = j;
											break;
										};
									}
								} else if( mode == find_prev_mode ){
									for( i64 j = selection[ i ].cursor - 1; j >= 0; j -= 1 ){
										if( strncmp( &file_buffer[ j ], &input_buffer[ start_index ], key_bytes ) == 0 ){
											selection[ i ].anchor = selection[ i ].cursor;
											selection[ i ].cursor = j;
											break;
										};
									}
								} else if( mode == append_find_next_mode ){
									for( i64 j = selection[ i ].cursor + 1; j < file_count - 1; j += 1 ){
										if( strncmp( &file_buffer[ j ], &input_buffer[ start_index ], key_bytes ) == 0 ){
											selection[ i ].cursor = j;
											break;
										};
									}
								} else if( mode == append_find_prev_mode ){
									for( i64 j = selection[ i ].cursor - 1; j >= 0; j -= 1 ){
										if( strncmp( &file_buffer[ j ], &input_buffer[ start_index ], key_bytes ) == 0 ){
											selection[ i ].cursor = j;
											break;
										};
									}
								}
							}
							command_count -= 1;
						} while( command_count > 0 );
						command_count = 0;
					}
					mode = command_mode;
				} else {
					assert( 0 && "invalid mode" );
				}
			}
loop_end:
			assert( input_index == input_count );
			assert( input_index - input_clip > 0 );
			memmove( input_buffer, &input_buffer[ input_count - input_clip ], input_clip );
			input_count = input_clip;
		}
		deoverlap_selections();
	}
	error( "Somehow broke out of main loop" );
}
