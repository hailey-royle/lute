/*

## File Overview

0.   File information
1.   Includes
2.   #defines, types
3.   Bar function declarations
4.   Command function declarations
5.   #include config.h
6.   global variables
7.   general utility functions
8.   abstract file/clipboard/selection/history data manipulation functions
9.   command function implmentations
10.  bar function implmentations
11.  input validation functions
12.  main

### Overview

### Commands

- Commands should not call other commands.
- Any command that can do work multiple times in a row should use 'command_count'.
	- A 'command_count' of zero should be ignored, and the work done once.
	- Some commands ( like 'command_delete' ) can't do work multiple times in a row.
	- Some commands ( like 'command_count_goto' ) use 'command_count' in other ways.
- A command end with 'command_count' being zero regardless of if it used it or not.
	- Unless it adds to the 'command_count'
- The clipboard data for the primary selection should move with the primary selection
	- Unless the primary selection is being changed
- If the number of selections remains the same, the clipboard data should too.
- If the number of selections changes, the clipboard data of all selections should be set to the clipboard data of the primary selection.
	- Unless a selection gets culled by 'clip_selection_overlap', then that selections data gets removed.

*/

#define _XOPEN_SOURCE 500 /* snprintf */

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

#define ansi_background_black "\x1b[40m"
#define ansi_background_blue "\x1b[44m"
#define ansi_background_cyan "\x1b[46m"
#define ansi_background_default "\x1b[49m"
#define ansi_background_green "\x1b[42m"
#define ansi_background_magenta "\x1b[45m"
#define ansi_background_red "\x1b[41m"
#define ansi_background_rgb( r, g, b ) "\x1b[48;2;" r ";" g ";" b "m"
#define ansi_background_white "\x1b[47m"
#define ansi_background_yellow "\x1b[43m"
#define ansi_blinking_end "\x1b[25m"
#define ansi_blinking_start "\x1b[5m"
#define ansi_bold_end "\x1b[22m"
#define ansi_bold_start "\x1b[1m"
#define ansi_cursor_hidden "\x1b[?25l"
#define ansi_cursor_home "\x1b[H"
#define ansi_cursor_show "\x1b[?25h"
#define ansi_dim_end "\x1b[22m"
#define ansi_dim_start "\x1b[2m"
#define ansi_end_alt_screen "\x1b[?1049l"
#define ansi_erase_cursor_to_end "\x1b[0J"
#define ansi_erase_cursor_to_end_line "\x1b[0K"
#define ansi_erase_cursor_to_start "\x1b[1J"
#define ansi_erase_cursor_to_start_line "\x1b[1K"
#define ansi_erase_line "\x1b[2K"
#define ansi_erase_saved_lines "\x1b[3J"
#define ansi_erase_screen "\x1b[2J"
#define ansi_foreground_black "\x1b[30m"
#define ansi_foreground_blue "\x1b[34m"
#define ansi_foreground_cyan "\x1b[36m"
#define ansi_foreground_default "\x1b[39m"
#define ansi_foreground_green "\x1b[32m"
#define ansi_foreground_magenta "\x1b[35m"
#define ansi_foreground_red "\x1b[31m"
#define ansi_foreground_rgb( r, g, b ) "\x1b[38;2;" r ";" g ";" b "m"
#define ansi_foreground_white "\x1b[37m"
#define ansi_foreground_yellow "\x1b[33m"
#define ansi_inverse_end "\x1b[27m"
#define ansi_inverse_start "\x1b[7m"
#define ansi_invisible_end "\x1b[28m"
#define ansi_invisible_start "\x1b[8m"
#define ansi_italic_end "\x1b[23m"
#define ansi_italic_start "\x1b[3m"
#define ansi_move_column( count ) "\x1b[" count "G"
#define ansi_move_down( count ) "\x1b[" count "B"
#define ansi_move_left( count ) "\x1b[" count "D"
#define ansi_move_line_start_down( count ) "\x1b[" count "F"
#define ansi_move_line_start_up( count ) "\x1b[" count "E"
#define ansi_move_line_up_scroll "\x1b M"
#define ansi_move_rigth( count ) "\x1b[" count "C"
#define ansi_move_to_line_column( line, column ) "\x1b[" line ";" column "H"
#define ansi_move_up( count ) "\x1b[" count "A"
#define ansi_request_cursor_possition "\x1b[6n"
#define ansi_reset_graphics "\x1b[0m"
#define ansi_restore_cursor_position "\x1b 8"
#define ansi_restore_screen "\x1b[?47l"
#define ansi_save_cursor_position "\x1b 7"
#define ansi_save_screen "\x1b[?47h"
#define ansi_start_alt_screen "\x1b[?1049h"
#define ansi_strikethrough_end "\x1b[29m"
#define ansi_strikethrough_start "\x1b[9m"
#define ansi_underline_end "\x1b[24m"
#define ansi_underline_start "\x1b[4m"

#define key_alt_0 "\x1b0"
#define key_alt_1 "\x1b1"
#define key_alt_2 "\x1b2"
#define key_alt_3 "\x1b3"
#define key_alt_4 "\x1b4"
#define key_alt_5 "\x1b5"
#define key_alt_6 "\x1b6"
#define key_alt_7 "\x1b7"
#define key_alt_8 "\x1b8"
#define key_alt_9 "\x1b9"
#define key_alt_A "\x1bA"
#define key_alt_B "\x1bB"
#define key_alt_C "\x1bC"
#define key_alt_D "\x1bD"
#define key_alt_E "\x1bE"
#define key_alt_F "\x1bF"
#define key_alt_G "\x1bG"
#define key_alt_H "\x1bH"
#define key_alt_I "\x1bI"
#define key_alt_J "\x1bJ"
#define key_alt_K "\x1bK"
#define key_alt_L "\x1bL"
#define key_alt_M "\x1bM"
#define key_alt_N "\x1bN"
#define key_alt_O "\x1bO"
#define key_alt_P "\x1bP"
#define key_alt_Q "\x1bQ"
#define key_alt_R "\x1bR"
#define key_alt_S "\x1bS"
#define key_alt_T "\x1bT"
#define key_alt_U "\x1bU"
#define key_alt_V "\x1bV"
#define key_alt_W "\x1bW"
#define key_alt_X "\x1bX"
#define key_alt_Y "\x1bY"
#define key_alt_Z "\x1bZ"
#define key_alt_a "\x1ba"
#define key_alt_ampersand "\x1b&"
#define key_alt_at "\x1b@"
#define key_alt_b "\x1bb"
#define key_alt_back_space "\x1b\x7f"
#define key_alt_backslash "\x1b\\"
#define key_alt_c "\x1bc"
#define key_alt_caret "\x1b^"
#define key_alt_close_bracked "\x1b]"
#define key_alt_close_parenthesis "\x1b)"
#define key_alt_closed_curly_bracket "\x1b}"
#define key_alt_colon "\x1b:"
#define key_alt_comma "\x1b,"
#define key_alt_d "\x1bd"
#define key_alt_dash "\x1b-"
#define key_alt_delete "\x1b[3;3~"
#define key_alt_dollar "\x1b$"
#define key_alt_double_quote "\x1b\""
#define key_alt_down_arrow "\x1b[1;3B"
#define key_alt_e "\x1be"
#define key_alt_end "\x1b[1;3F"
#define key_alt_enter "\x1b\xa"
#define key_alt_equals "\x1b="
#define key_alt_escape "\x1b\x1b"
#define key_alt_exclamation_point "\x1b!"
#define key_alt_f "\x1bf"
#define key_alt_g "\x1bg"
#define key_alt_greater_then "\x1b>"
#define key_alt_h "\x1bh"
#define key_alt_hashtag "\x1b#"
#define key_alt_home "\x1b[1;3H"
#define key_alt_i "\x1bi"
#define key_alt_j "\x1bj"
#define key_alt_k "\x1bk"
#define key_alt_l "\x1bl"
#define key_alt_left_arrow "\x1b[1;3D"
#define key_alt_less_then "\x1b<"
#define key_alt_m "\x1bm"
#define key_alt_n "\x1bn"
#define key_alt_o "\x1bo"
#define key_alt_open_bracket "\x1b["
#define key_alt_open_curly_bracket "\x1b{"
#define key_alt_open_parenthesis "\x1b("
#define key_alt_p "\x1bp"
#define key_alt_page_down "\x1b[6;3~"
#define key_alt_page_up "\x1b[5;3~"
#define key_alt_percent "\x1b%"
#define key_alt_period "\x1b."
#define key_alt_pipe "\x1b|"
#define key_alt_plus "\x1b+"
#define key_alt_q "\x1bq"
#define key_alt_question_mark "\x1b?"
#define key_alt_r "\x1br"
#define key_alt_right_arrow "\x1b[1;3C"
#define key_alt_s "\x1bs"
#define key_alt_semi_colon "\x1b;"
#define key_alt_single_quote "\x1b'"
#define key_alt_slash "\x1b/"
#define key_alt_space "\x1b "
#define key_alt_star "\x1b*"
#define key_alt_t "\x1bt"
#define key_alt_tab "\x1b\x9"
#define key_alt_u "\x1bu"
#define key_alt_underscore "\x1b_"
#define key_alt_up_arrow "\x1b[1;3A"
#define key_alt_v "\x1bv"
#define key_alt_w "\x1bw"
#define key_alt_x "\x1bx"
#define key_alt_y "\x1by"
#define key_alt_z "\x1bz"
#define key_backspace "\x7f"
#define key_control_2 "\x0"
#define key_control_3 "\x1b"
#define key_control_4 "\x1c"
#define key_control_5 "\x1d"
#define key_control_6 "\x1e"
#define key_control_7 "\x1f"
#define key_control_a "\x1"
#define key_control_alt_3 "\x1b\x1b"
#define key_control_alt_4 "\x1b\x1c"
#define key_control_alt_5 "\x1b\x1d"
#define key_control_alt_6 "\x1b\x1e"
#define key_control_alt_7 "\x1b\x1f"
#define key_control_alt_8 "\x1b\x7f"
#define key_control_alt_backslash "\x1b\x1c"
#define key_control_alt_backspace "\x1b\x8"
#define key_control_alt_close_bracket "\x1b\x1d"
#define key_control_alt_delete "\x1b[3;7~"
#define key_control_alt_down_arrow "\x1b[1;7B"
#define key_control_alt_end "\x1b[1;7F"
#define key_control_alt_home "\x1b[1;7H"
#define key_control_alt_left_arrow "\x1b[1;7D"
#define key_control_alt_open_bracket "\x1b\x1b"
#define key_control_alt_page_down "\x1b[6;7~"
#define key_control_alt_page_up "\x1b[5;7~"
#define key_control_alt_right_arrow "\x1b[1;7C"
#define key_control_alt_space "\x1b\x0"
#define key_control_alt_underscore "\x1b\x1f"
#define key_control_alt_up_arrow "\x1b[1;7A"
#define key_control_b "\x2"
#define key_control_backslash "\x1c"
#define key_control_backspace "\x8"
#define key_control_c "\x3"
#define key_control_caret "\x1e"
#define key_control_close_bracket "\x1d"
#define key_control_d "\x4"
#define key_control_delete "\x1b[3;5~"
#define key_control_down_arrow "\x1b[1;5B"
#define key_control_e "\x5"
#define key_control_end "\x1b[1;5F"
#define key_control_f "\x6"
#define key_control_g "\x7"
#define key_control_h "\x8"
#define key_control_home "\x1b[1;5H"
#define key_control_i "\x9"
#define key_control_j "\xa"
#define key_control_k "\xb"
#define key_control_l "\xc"
#define key_control_left_arrow "\x1b[1;5D"
#define key_control_m "\xa"
#define key_control_n "\xe"
#define key_control_o "\xf"
#define key_control_open_bracket "\x1b"
#define key_control_p "\x10"
#define key_control_page_down "\x1b[6;5~"
#define key_control_page_up "\x1b[5;5~"
#define key_control_q "\x11"
#define key_control_r "\x12"
#define key_control_right_arrow "\x1b[1;5C"
#define key_control_s "\x13"
#define key_control_shift_alt_delete "\x1b[3;8~"
#define key_control_shift_alt_down_arrow "\x1b[1;8B"
#define key_control_shift_alt_end "\x1b[1;8F"
#define key_control_shift_alt_home "\x1b[1;8H"
#define key_control_shift_alt_left_arrow "\x1b[1;8D"
#define key_control_shift_alt_page_down "\x1b[6;8~"
#define key_control_shift_alt_page_up "\x1b[5;8~"
#define key_control_shift_alt_right_arrow "\x1b[1;8C"
#define key_control_shift_alt_up_arrow "\x1b[1;8A"
#define key_control_shift_delete "\x1b[3;6~"
#define key_control_shift_down_arrow "\x1b[1;6B"
#define key_control_shift_end "\x1b[1;6F"
#define key_control_shift_home "\x1b[1;6H"
#define key_control_shift_left_arrow "\x1b[1;6D"
#define key_control_shift_page_down "\x1b[6;6~"
#define key_control_shift_page_up "\x1b[5;6~"
#define key_control_shift_right_arrow "\x1b[1;6C"
#define key_control_shift_up_arrow "\x1b[1;6A"
#define key_control_space "\x0"
#define key_control_t "\x14"
#define key_control_u "\x15"
#define key_control_underscore "\x1f"
#define key_control_up_arrow "\x1b[1;5A"
#define key_control_v "\x16"
#define key_control_w "\x17"
#define key_control_x "\x18"
#define key_control_y "\x19"
#define key_control_z "\x1a"
#define key_delete "\x1b[3~"
#define key_down_arrow "\x1b[B"
#define key_end "\x1b[F"
#define key_enter "\xa"
#define key_escape "\x1b"
#define key_home "\x1b[H"
#define key_left_arrow "\x1b[D"
#define key_page_down "\x1b[6~"
#define key_page_up "\x1b[5~"
#define key_right_arrow "\x1b[C"
#define key_shift_alt_delete "\x1b[3;4~"
#define key_shift_alt_down_arrow "\x1b[1;4B"
#define key_shift_alt_end "\x1b[1;4F"
#define key_shift_alt_home "\x1b[1;4H"
#define key_shift_alt_left_arrow "\x1b[1;4D"
#define key_shift_alt_page_down "\x1b[6;4~"
#define key_shift_alt_page_up "\x1b[5;4~"
#define key_shift_alt_right_arrow "\x1b[1;4C"
#define key_shift_alt_up_arrow "\x1b[1;4A"
#define key_shift_delete "\x1b[3;2~"
#define key_shift_down_arrow "\x1b[1;2B"
#define key_shift_end "\x1b[1;2F"
#define key_shift_home "\x1b[1;2H"
#define key_shift_left_arrow "\x1b[1;2D"
#define key_shift_page_down "\x1b[6;2~"
#define key_shift_page_up "\x1b[5;2~"
#define key_shift_right_arrow "\x1b[1;2C"
#define key_shift_up_arrow "\x1b[1;2A"
#define key_tab "\x9"
#define key_up_arrow "\x1b[A"

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
void command_search_input();
void command_search_next_primary();
void command_search_prev_primary();
void command_search_file_primary();
void command_split_newline();
void command_split_collapse();
void command_next_selection();
void command_prev_selection();
void command_move_char_next();
void command_move_char_prev();
void command_move_word_next();
void command_move_word_prev();
void command_move_line_next();
void command_move_line_prev();
void command_move_para_next();
void command_move_para_prev();
void command_move_pinned_char_next();
void command_move_pinned_char_prev();
void command_move_pinned_word_next();
void command_move_pinned_word_prev();
void command_move_pinned_line_next();
void command_move_pinned_line_prev();
void command_move_pinned_para_next();
void command_move_pinned_para_prev();
void command_move_find_next();
void command_move_find_prev();
void command_move_pinned_find_next();
void command_move_pinned_find_prev();
void command_move_line_end();
void command_move_line_start();
void command_move_file_end();
void command_move_file_start();
void command_move_pinned_line_end();
void command_move_pinned_line_start();
void command_move_pinned_file_start();
void command_move_pinned_file_end();
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

char* file_name = NULL;
char file_buffer[ max_file_bytes ];
i64 file_count = 0;

i32 screen_cols = 0;
i32 screen_rows = 0;
char frame_buffer[ max_frame_bytes ];
i64 frame_count = 0;

char input_buffer[ max_input_bytes ];
i64 input_count = 0;

char search_buffer[ max_search_bytes ];
i64 search_count = 0;

char clipboard_buffer[ max_clipboard_bytes ];
i64 clipboard_count = 0;
selection_struct selection[ max_selection_count ];
i64 selection_count = 1;  /* first selection is initalized to all zeros */
i64 primary_selection_index = 0;

char edit_buffer[ max_edit_bytes ];
i64 edit_count = 0;
edit history[ max_edit_count ];
i64 undo_count = 0;
i64 redo_count = 0;

i8 mode = command_mode;
i64 command_count = 0;
i8 file_modified = 0;
char* warning = NULL;

struct termios cache_termios;
i8 terminal_state_modified = 0;

void terminal_reset(){
/* resets the terminal to its state before calling terminal_state_modified */
	if( terminal_state_modified ){
		tcsetattr( STDIN_FILENO, TCSAFLUSH, &cache_termios );
		write( STDOUT_FILENO, ansi_end_alt_screen ansi_cursor_show, strlen( ansi_end_alt_screen ansi_cursor_show ));
	}
}

void assert_failed( char* file, i32 line, const char* func, char* expression ){
	terminal_reset();
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
	terminal_reset();
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

void terminal_setup(){
/*
  Set up the terminal state for the rest of the program.
  Sets terminal_state_modified to 1 so 'terminal_reset' does not erroneously modify the terminal.
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
	terminal_state_modified = 1;
}

void terminal_get_dimensions(){
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

void terminal_read_file(){
/*
  If the file can be opened and edited, then read the file into file_buffer.
  If the file does not exist, then create a new file, adding one '\n'.
  If the last byte is not '\n', then add it.
  Note: all lines must end in a '\n', and there must be at least one line.
*/
	if( access( file_name, F_OK ) == 0 ){
		i32 fd = open( file_name, O_RDWR | O_CREAT );
		if( fd < 0 ){
			error( "The file '%s' could not be opened." );
		}
		struct stat sb;
		if( fstat( fd, &sb ) < 0 ){
			error( "Could not get file '%s' type." );
		}
		if(( sb.st_mode & 0170000 /* S_IFMT */ ) == 0100000 /* S_IFREG */ ){
			file_count = read( fd, file_buffer, max_file_bytes );
			if( file_count < 0 ){
				error( "The file '%s' could not be read.", file_name );
			}
			if( max_file_bytes <= file_count ){
				error( "The file '%s' is larger then 'max_file_bytes'.", file_name );
			}
		} else {
			error( "'%s' is not a regular file.", file_name );
		}
		close( fd );
	}
	if(( file_count == 0 ) || ( file_buffer[ file_count - 1 ] != '\n' )){
		if( max_file_bytes <= file_count + 1 ){
			error( "The file '%s' can not fit in 'max_file_bytes'.", file_name );
		}
		file_buffer[ file_count ] = '\n';
		file_count += 1;
	}
}

void terminal_write_file(){
	i32 fd = open( file_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
	if( fd < 0 ){
		error( "The file '%s' could not be opened." );
	}
	i64 write_bytes = write( fd, file_buffer, file_count );
	if( write_bytes < 0 ){
		error( "The file '%s' could not be written.", file_name );
	}
	close( fd );
}

void terminal_draw_frame(){
	write( STDOUT_FILENO, frame_buffer, frame_count );
}

i32 utf8_next_length( char* src ){
	assert( src != NULL );
	if(( *src & 0x80 ) == 0 ){  /* ascii */
		return 1;
	} else if(( *src & 0xe0 ) == 0xc0 ){  /* two byte unicode */
		return 2;
	} else if(( *src & 0xf0 ) == 0xe0 ){  /* three byte unicode */
		return 3;
	} else if(( *src & 0xf8 ) == 0xf0 ){  /* four byte unicode */
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
	while(( *src & 0xc0 ) == 0x80 ){  /* while is utf8 continuation byte */
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
	i32 i = 0;
	while( i < index ){
		if( buffer[ i ] == '\n' ){
			line += 1;
		}
		i += 1;
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
	i64 i = selection_min( index );
	while( i < selection_max( index )){
		length += 1;
		i += utf8_next_length( &file_buffer[ i ]);
	}
	return length;
}

void delete_all_non_primary_selections(){
	assert( selection_count > 0 );
	assert( primary_selection_index >= 0 );
	if( selection_count == 1 ){
		assert( primary_selection_index == 0 );
		return;
	}
	if( primary_selection_index != 0 ){
		i32 i = 0;
		i64 clipboard_index = 0;
		while( i < primary_selection_index ){
			clipboard_index += selection[ i ].clipboard_count;
			i += 1;
		}
		memmove( clipboard_buffer, &clipboard_buffer[ clipboard_index ], selection[ primary_selection_index ].clipboard_count );
		selection[ 0 ] = selection[ primary_selection_index ];
	}
	selection_count = 1;
	clipboard_count = selection[ 0 ].clipboard_count;
	primary_selection_index = 0;
}

void set_all_selection_clipboards_to_primary(){
	if( primary_selection_index != 0 ){
		i32 i = 0;
		i64 clipboard_index = 0;
		while( i < primary_selection_index ){
			clipboard_index += selection[ i ].clipboard_count;
			i += 1;
		}
		memmove( clipboard_buffer, &clipboard_buffer[ clipboard_index ], selection[ primary_selection_index ].clipboard_count );
		selection[ 0 ].clipboard_count = selection[ primary_selection_index ].clipboard_count;
	}
	primary_selection_index = 0;
	clipboard_count = selection[ primary_selection_index ].clipboard_count;
	i32 i = 1;
	while( i < selection_count ){
		selection[ i ].clipboard_count = selection[ primary_selection_index ].clipboard_count;
		if( max_clipboard_bytes <= clipboard_count + selection[ i ].clipboard_count ){
			error( "Clipboard count overflow, increase max_clipboard_count" );
		}
		buffer_append( clipboard_buffer, &clipboard_count, clipboard_buffer, selection[ primary_selection_index ].clipboard_count );
		i += 1;
	}
}

void new_undo(){
	redo_count = 0;
	i32 i = 0;
	while( i < selection_count ){
		if( max_edit_count <= undo_count + 1 ){
			error( "Edit count overflow, increase max_edit_count" );
		}
		history[ undo_count ].insert_count = 0;
		history[ undo_count ].delete_count = 0;
		history[ undo_count ].selection_index = i;
		history[ undo_count ].index = selection_min( i );
		undo_count += 1;
		i += 1;
	}
}

void selection_insert( char* insert, i64 insert_bytes ){
	assert( insert != NULL );
	assert( insert_bytes > 0 );
	file_modified = 1;
	i64 edit_index = 0;
	i32 i = 0;
	while( i < undo_count - selection_count ){
		edit_index += history[ i ].insert_count;
		edit_index += history[ i ].delete_count;
		i += 1;
	}
	i64 clipboard_index = 0;
	i = 0;
	while( i < selection_count ){
/* edit history */
		i64 history_index = undo_count - selection_count + i;
		if( max_edit_bytes <= edit_count + insert_bytes ){
			error( "Edit buffer overflow, increase max_edit_bytes" );
		}
		buffer_insert( edit_buffer, &edit_count, edit_index + history[ history_index ].insert_count, insert, insert_bytes );
		history[ history_index ].insert_count += insert_bytes;
		edit_index += history[ history_index ].insert_count;
		edit_index += history[ history_index ].delete_count;
/* clipboard */
		if( max_clipboard_bytes <= clipboard_count + insert_bytes ){
			error( "Clipboard buffer overflow, increase max_clipboard_bytes" );
		}
		buffer_insert( clipboard_buffer, &clipboard_count, clipboard_index + selection[ i ].clipboard_count, insert, insert_bytes );
		selection[ i ].clipboard_count += insert_bytes;
		clipboard_index += selection[ i ].clipboard_count;
/* selection */
		selection[ i ].cursor += i * insert_bytes;
		if( max_file_bytes <= file_count + insert_bytes ){
			error( "File buffer overflow, increase max_file_bytes" );
		}
		buffer_insert( file_buffer, &file_count, selection[ i ].cursor, insert, insert_bytes );
		selection[ i ].cursor += insert_bytes;
		selection[ i ].anchor = selection[ i ].cursor;
		i += 1;
	}
}

void selection_delete(){
	file_modified = 1;
	i64 total_deleted = 0;
	i64 edit_index = 0;
	i32 i = 0;
	while( i < undo_count - selection_count ){
		edit_index += history[ i ].insert_count;
		edit_index += history[ i ].delete_count;
		i += 1;
	}
	i = 0;
	while( i < selection_count ){
		i64 delete_bytes = selection_bytes( i );
		i64 selection_delete_index = selection_min( i ) - total_deleted;
		total_deleted += delete_bytes;
/* edit history */
		i64 history_index = undo_count - selection_count + i;
		assert( i == history[ history_index ].selection_index );
		edit_index += history[ history_index ].insert_count;
		if( max_edit_bytes <= edit_count + delete_bytes ){
			error( "Edit buffer overflow, increase max_edit_bytes" );
		}
		assert( history[ history_index ].delete_count == 0 );
		buffer_insert( edit_buffer, &edit_count, edit_index, &file_buffer[ selection_delete_index ], delete_bytes );
		history[ history_index ].delete_count += delete_bytes;
		edit_index += history[ history_index ].delete_count;
/* file */
		buffer_delete( file_buffer, &file_count, selection_delete_index, delete_bytes );
		selection[ i ].cursor = selection_delete_index;
		selection[ i ].anchor = selection[ i ].cursor;
		i += 1;
	}
}

void selection_delete_backspace(){
	file_modified = 1;
	i64 total_deleted = 0;
	i64 edit_index = 0;
	i32 i = 0;
	while( i < undo_count - selection_count ){
		edit_index += history[ i ].insert_count;
		edit_index += history[ i ].delete_count;
		i += 1;
	}
	i64 clipboard_index = 0;
	i = 0;
	while( i < selection_count ){
		if( selection[ i ].cursor > 0 ){
			i64 delete_bytes = utf8_prev_length( &file_buffer[ selection[ i ].cursor - total_deleted ]); 
			total_deleted += delete_bytes;
/* edit history */
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
/* clipboard */
			if( clipboard_count - delete_bytes >= 0 ){
				selection[ i ].clipboard_count -= delete_bytes;
				buffer_delete( clipboard_buffer, &clipboard_count, clipboard_index + selection[ i ].clipboard_count, delete_bytes );
			}
			clipboard_index += selection[ i ].clipboard_count;
/* selection */
			selection[ i ].cursor -= total_deleted;
			selection[ i ].anchor = selection[ i ].cursor;
			buffer_delete( file_buffer, &file_count, selection[ i ].cursor, delete_bytes );
		}
		i += 1;
	}
}

void selection_copy(){
	clipboard_count = 0;
	i32 i = 0;
	while( i < selection_count ){
		selection[ i ].clipboard_count = selection_bytes( i );
		if( max_clipboard_bytes <= clipboard_count + selection[ i ].clipboard_count ){
			error( "Clipboard buffer overflow, increase max_clipboard_bytes" );
		}
		buffer_append( clipboard_buffer, &clipboard_count, &file_buffer[ selection_min( i ) ], selection[ i ].clipboard_count );
		i += 1;
	}
}

void selection_paste(){
	file_modified = 1;
	i64 edit_index = 0;
	i32 i = 0;
	while( i < undo_count - selection_count ){
		edit_index += history[ i ].insert_count;
		edit_index += history[ i ].delete_count;
		i += 1;
	}
	i64 clipboard_index = 0;
	i = 0;
	while( i < selection_count ){
/* history */
		i64 history_index = undo_count - selection_count + i;
		assert( i == history[ history_index ].selection_index );
		edit_index += history[ history_index ].insert_count;
		if( max_edit_bytes <= edit_count + selection[ i ].clipboard_count ){
			error( "Edit buffer overflow, increase max_edit_bytes" );
		}
		buffer_insert( edit_buffer, &edit_count, edit_index, &clipboard_buffer[ clipboard_index ], selection[ i ].clipboard_count );
		history[ history_index ].insert_count += selection[ i ].clipboard_count;
		edit_index += history[ history_index ].delete_count;
/* selection */
		selection[ i ].cursor += clipboard_index;
		selection[ i ].anchor = selection[ i ].cursor + selection[ i ].clipboard_count;
		if( max_file_bytes <= file_count + clipboard_index ){
			error( "File buffer overflow, increase max_file_bytes" );
		}
		buffer_insert( file_buffer, &file_count, selection[ i ].cursor, &clipboard_buffer[ clipboard_index ], selection[ i ].clipboard_count );
		clipboard_index += selection[ i ].clipboard_count;
		i += 1;
	}
}

void select_cursor_line(){
	i32 i = 0;
	while( i < selection_count ){
		selection[ i ].anchor = selection[ i ].cursor + 1;
		while(( selection[ i ].anchor < file_count - 1 ) && ( file_buffer[ selection[ i ].anchor - 1 ] != '\n' )){
			selection[ i ].anchor += 1;
		}
		while(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] != '\n' )){
			selection[ i ].cursor -= 1;
		}
		i += 1;
	}
}

void select_inside( char* left, i64 left_count, char* right, i64 right_count ){
	i32 i = 0;
	while( i < selection_count ){
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
		i += 1;
	}
}

void selection_split( char* select, i64 select_bytes ){
	assert( select != NULL );
	assert( select_bytes >= 0 );
	if( select_bytes == 0 ){
		return;
	}
	delete_all_non_primary_selections();
	i64 min = selection_min( primary_selection_index );
	i64 max = selection_max( primary_selection_index );
	i64 new_selection_count = 0;
	while( min <= max - select_bytes ){
		if( strncmp( &file_buffer[ min ], select, select_bytes ) == 0 ){
			if( max_selection_count <= new_selection_count + 1 ){
				error( "Selection count overflow, increase max_selection_count" );
			}
			selection[ new_selection_count ].cursor = min;
			selection[ new_selection_count ].anchor = selection[ new_selection_count ].cursor + select_bytes;
			new_selection_count += 1;
		}
		min += 1;
	}
	assert( new_selection_count >= 0 );
	if(( new_selection_count > 0 ) && ( new_selection_count != selection_count )){
		selection_count = new_selection_count;
		set_all_selection_clipboards_to_primary();
	}
}

void clip_selection_overlap(){
	i32 i = 0;
	while( i < selection_count ){
		i64 selection_min = ( selection[ i ].cursor < selection[ i ].anchor ) ? selection[ i ].cursor : selection[ i ].anchor;
		i64 selection_max = ( selection[ i ].cursor > selection[ i ].anchor ) ? selection[ i ].cursor : selection[ i ].anchor;
		i32 j = selection_count - 1;
		while( j >= 0 ){
			if( i != j ){
				i8 j_cursor_inside = (( selection[ j ].cursor >= selection_min ) && ( selection[ j ].cursor <= selection_max )) ? 1 : 0;
				i8 j_anchor_inside = (( selection[ j ].anchor >= selection_min ) && ( selection[ j ].anchor <= selection_max )) ? 1 : 0;
				if( j_anchor_inside && j_cursor_inside ){  /* cull j */
					if( j < primary_selection_index ){
						primary_selection_index -= 1;
					} else if( j == primary_selection_index ){
						primary_selection_index = ( i < j ) ? i : j;
					}
					i64 clipboard_index = 0;
					i32 h = 0;
					while( h < j ){
						clipboard_index += selection[ h ].clipboard_count;
						h += 1;
					}
					buffer_delete( clipboard_buffer, &clipboard_count, clipboard_index, selection[ j ].clipboard_count );
					h = j;
					while( h < selection_count ){
						selection[ h ] = selection[ h + 1 ];
						h += 1;
					}
					selection_count -= 1;
				} else if( j_anchor_inside ){
					selection[ j ].anchor = selection[ i ].cursor;
				}
			}
			j -= 1;
		}
		i += 1;
	}
}

void command_quit(){
	terminal_reset();
	exit( 1 );
}

void command_write(){
	terminal_write_file();
	file_modified = 0;
}

void command_write_quit(){
	terminal_write_file();
	file_modified = 0;
	terminal_reset();
	exit( 1 );
}

void command_edit_mode(){
	mode = edit_mode;
	command_count = 0;
	clipboard_count = 0;
	i32 i = 0;
	while( i < selection_count ){
		selection[ i ].clipboard_count = 0;
		selection[ i ].anchor = selection[ i ].cursor;
		i += 1;
	}
	new_undo();
}

void command_edit_newline(){
	command_move_line_end();
	clip_selection_overlap();
	mode = edit_mode;
	command_count = 0;
	clipboard_count = 0;
	i32 i = 0;
	while( i < selection_count ){
		selection[ i ].clipboard_count = 0;
		selection[ i ].anchor = selection[ i ].cursor;
		i += 1;
	}
	new_undo();
	selection_insert( "\n", 1 );
}

void command_indent(){
	command_count = 0;
	command_move_line_start();
	i32 i = 0;
	while( i < selection_count ){
		selection[ i ].anchor = selection[ i ].cursor;
		i += 1;
	}
	clip_selection_overlap();
	new_undo();
	selection_insert( tab_chars, strlen( tab_chars ));
	command_move_line_start();
}

void command_deindent(){
	command_count = 0;
	command_move_line_start();
	i32 i = 0;
	while( i < selection_count ){
		if( file_buffer[ selection[ i ].cursor ] == '\t' ){
			selection[ i ].anchor = selection[ i ].cursor + 1;
		} else {
			selection[ i ].anchor = selection[ i ].cursor;
		}
		i += 1;
	}
	clip_selection_overlap();
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
	assert( new_selection_count > 0 );
	undo_count -= new_selection_count;
	redo_count += new_selection_count;
	assert( undo_count >= 0 );
	i64 edit_index = 0;
	i32 i = 0;
	while( i < undo_count ){
		edit_index += history[ i ].insert_count;
		edit_index += history[ i ].delete_count;
		i += 1;
	}
	i = 0;
	while( i < new_selection_count ){
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
		i += 1;
	}
	if( new_selection_count != selection_count ){
		selection_count = new_selection_count;
		set_all_selection_clipboards_to_primary();
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
	i32 i = 0;
	while( i < undo_count ){
		edit_index += history[ i ].insert_count;
		edit_index += history[ i ].delete_count;
		i += 1;
	}
	i64 file_edit_offset = 0;
	i = 0;
	while( i < new_selection_count ){
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
		i += 1;
	}
	undo_count += new_selection_count;
	redo_count -= new_selection_count;
	assert( redo_count >= 0 );
	if( new_selection_count != selection_count ){
		selection_count = new_selection_count;
		set_all_selection_clipboards_to_primary();
	}
}

void command_paste(){
	command_count = 0;
	i32 i = 0;
	while( i < selection_count ){
		selection[ i ].anchor = selection[ i ].cursor;
		i += 1;
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
	i32 i = 0;
	while( i < selection_count ){
		selection[ i ].clipboard_count = 0;
		selection[ i ].anchor = selection[ i ].cursor;
		i += 1;
	}
}

void command_replace(){
	command_count = 0;
	new_undo();
	selection_delete();
	selection_paste();
}

void command_line_copy(){
	command_count = 0;
	select_cursor_line();
	selection_copy();
}

void command_line_delete(){
	command_count = 0;
	select_cursor_line();
	selection_copy();
	new_undo();
	selection_delete();
}

void command_line_change(){
	command_count = 0;
	select_cursor_line();
	selection_copy();
	new_undo();
	selection_delete();
	mode = edit_mode;
	clipboard_count = 0;
	i32 i = 0;
	while( i < selection_count ){
		selection[ i ].clipboard_count = 0;
		selection[ i ].anchor = selection[ i ].cursor;
		i += 1;
	}
}

void command_line_replace(){
	command_count = 0;
	select_cursor_line();
	new_undo();
	selection_delete();
	selection_paste();
}

void command_search_input(){
	command_count = 0;
	mode = search_mode;
}

void command_search_next_primary(){
	i64 search_bytes = selection_bytes( primary_selection_index );
	i64 index = selection_max( selection_count - 1 );
	assert( search_bytes >= 0 );
	if( search_bytes == 0 ){
		return;
	}
	i64 clipboard_index = 0;
	i32 i = 0;
	while( i < primary_selection_index ){
		clipboard_index += selection[ i ].clipboard_count;
		i += 1;
	}
	while( index < file_count - 1 - search_bytes ){
		if( strncmp( &file_buffer[ index ], &file_buffer[ selection_min( primary_selection_index )], search_bytes ) == 0 ){
			if( max_selection_count <= selection_count + 1 ){
				error( "Selection count overflow, increase max_selection_count" );
			}
			if( selection[ primary_selection_index ].cursor < selection[ primary_selection_index ].anchor ){
				selection[ selection_count ].cursor = index;
				selection[ selection_count ].anchor = selection[ selection_count ].cursor + search_bytes;
			} else {
				selection[ selection_count ].anchor = index;
				selection[ selection_count ].cursor = selection[ selection_count ].anchor + search_bytes;
			}
			selection[ selection_count ].clipboard_count = selection[ primary_selection_index ].clipboard_count;
			if( max_clipboard_bytes <= clipboard_count + selection[ selection_count ].clipboard_count ){
				error( "Clipboard count overflow, increase max_clipboard_count" );
			}
			buffer_append( clipboard_buffer, &clipboard_count, &clipboard_buffer[ clipboard_index ], selection[ primary_selection_index ].clipboard_count );
			selection_count += 1;
			break;
		}
		index += 1;
	}
}

void command_search_prev_primary(){
	i64 search_bytes = selection_bytes( primary_selection_index );
	i64 index = selection_min( 0 ) - search_bytes;
	assert( search_bytes >= 0 );
	if( search_bytes == 0 ){
		return;
	}
	i64 clipboard_index = 0;
	i32 i = 0;
	while( i < primary_selection_index ){
		clipboard_index += selection[ i ].clipboard_count;
		i += 1;
	}
	while( index >= 0 ){
		if( strncmp( &file_buffer[ index ], &file_buffer[ selection_min( primary_selection_index )], search_bytes ) == 0 ){
			if( max_selection_count <= selection_count + 1 ){
				error( "Selection count overflow, increase max_selection_count" );
			}
			i = selection_count;
			while( i >= 0 ){
				selection[ i + 1 ] = selection[ i ];
				i -= 1;
			}
			primary_selection_index += 1;
			if( selection[ primary_selection_index ].cursor < selection[ primary_selection_index ].anchor ){
				selection[ 0 ].cursor = index;
				selection[ 0 ].anchor = selection[ 0 ].cursor + search_bytes;
			} else {
				selection[ 0 ].anchor = index;
				selection[ 0 ].cursor = selection[ 0 ].anchor + search_bytes;
			}
			selection[ 0 ].clipboard_count = selection[ primary_selection_index ].clipboard_count;
			if( max_clipboard_bytes <= clipboard_count + selection[ 0 ].clipboard_count ){
				error( "Clipboard count overflow, increase max_clipboard_count" );
			}
			buffer_insert( clipboard_buffer, &clipboard_count, 0, &clipboard_buffer[ clipboard_index ], selection[ primary_selection_index ].clipboard_count );
			selection_count += 1;
			break;
		}
		index -= 1;
	}
}

void command_search_file_primary(){
	delete_all_non_primary_selections();
	i64 search_bytes = selection_bytes( primary_selection_index );
	i64 primary_index = selection_min( primary_selection_index );
	char* search = &file_buffer[ primary_index ];
	assert( search_bytes >= 0 );
	if( search_bytes == 0 ){
		return;
	}
	selection_count = 0;
	i64 index = 0;
	while( index < file_count - 1 - search_bytes ){
		if( strncmp( &file_buffer[ index ], search, search_bytes ) == 0 ){
			if( index == primary_index ){
				primary_selection_index = selection_count;
			}
			selection[ selection_count ].cursor = index;
			selection[ selection_count ].anchor = index + search_bytes;
			if( selection_count > 0 ){
				selection[ selection_count ].clipboard_count = selection[ 0 ].clipboard_count;
				if( max_clipboard_bytes <= clipboard_count + search_bytes ){
					error( "Clipboard count overflow, increase max_clipboard_count" );
				}
				buffer_append( clipboard_buffer, &clipboard_count, clipboard_buffer, selection[ 0 ].clipboard_count );
			}
			selection_count += 1;
		}
		index += 1;
	}
	assert( selection_count > 0 );
}

void command_split_newline(){
	command_count = 0;
	selection_split( "\n", 1 );
}

void command_split_collapse(){
	command_count = 0;
	delete_all_non_primary_selections();
}

void command_next_selection(){
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

void command_prev_selection(){
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
		i32 i = 0;
		while( i < selection_count ){
			selection[ i ].anchor = selection[ i ].cursor;
			if( selection[ i ].cursor < file_count - 1 ){
				selection[ i ].cursor += utf8_next_length( &file_buffer[ selection[ i ].cursor ]);
			}
			i += 1;
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_char_prev(){
	do {
		i32 i = 0;
		while( i < selection_count ){
			selection[ i ].anchor = selection[ i ].cursor;
			do {
				if( selection[ i ].cursor > 0 ){
					selection[ i ].cursor -= 1;
				}
			} while(( file_buffer[ selection[ i ].cursor ] & 0xc0 ) == 0x80 ); /* while is utf8_continuation_byte */
			i += 1;
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
		i32 i = 0;
		while( i < selection_count ){
			selection[ i ].anchor = selection[ i ].cursor;
			if(( selection[ i ].cursor < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor ] == '\n' )){
				selection[ i ].cursor += 1;
			} else {
				while(( selection[ i ].cursor < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor ] != '\n' ) && !is_word_whitespace( file_buffer[ selection[ i ].cursor ])){
					selection[ i ].cursor += 1;
				}
				while(( selection[ i ].cursor < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor ] != '\n' ) && is_word_whitespace( file_buffer[ selection[ i ].cursor ])){
					selection[ i ].cursor += 1;
				}
			}
			i += 1;
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_word_prev(){
	do {
		i32 i = 0;
		while( i < selection_count ){
			selection[ i ].anchor = selection[ i ].cursor;
			if(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] == '\n' )){
				selection[ i ].cursor -= 1;
			} else {
				while(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] != '\n' ) && is_word_whitespace( file_buffer[ selection[ i ].cursor - 1 ])){
					selection[ i ].cursor -= 1;
				}
				while(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] != '\n' ) && !is_word_whitespace( file_buffer[ selection[ i ].cursor - 1 ])){
					selection[ i ].cursor -= 1;
				}
			}
			i += 1;
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_line_next(){
	do {
		i32 i = 0;
		while( i < selection_count ){
			selection[ i ].anchor = selection[ i ].cursor;
			while(( selection[ i ].cursor < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor ] != '\n' )){
				selection[ i ].cursor += 1;
			}
			if( selection[ i ].cursor < file_count - 1 ){
				selection[ i ].cursor += 1;
			}
			i += 1;
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_line_prev(){
	do {
		i32 i = 0;
		while( i < selection_count ){
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
			i += 1;
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_para_next(){
	do {
		i32 i = 0;
		while( i < selection_count ){
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
			i += 1;
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_para_prev(){
	do {
		i32 i = 0;
		while( i < selection_count ){
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
			i += 1;
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_pinned_char_next(){
	do {
		i32 i = 0;
		while( i < selection_count ){
			if( selection[ i ].cursor < file_count - 1 ){
				selection[ i ].cursor += utf8_next_length( &file_buffer[ selection[ i ].cursor ]);
			}
			i += 1;
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_pinned_char_prev(){
	do {
		i32 i = 0;
		while( i < selection_count ){
			do {
				if( selection[ i ].cursor > 0 ){
					selection[ i ].cursor -= 1;
				}
			} while(( file_buffer[ selection[ i ].cursor ] & 0xc0 ) == 0x80 ); /* while is utf8_continuation_byte */
			i += 1;
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_pinned_word_next(){
	do {
		i32 i = 0;
		while( i < selection_count ){
			if(( selection[ i ].cursor < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor ] == '\n' )){
				selection[ i ].cursor += 1;
			} else {
				while(( selection[ i ].cursor < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor ] != '\n' ) && !is_word_whitespace( file_buffer[ selection[ i ].cursor ])){
					selection[ i ].cursor += 1;
				}
				while(( selection[ i ].cursor < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor ] != '\n' ) && is_word_whitespace( file_buffer[ selection[ i ].cursor ])){
					selection[ i ].cursor += 1;
				}
			}
			i += 1;
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_pinned_word_prev(){
	do {
		i32 i = 0;
		while( i < selection_count ){
			if(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] == '\n' )){
				selection[ i ].cursor -= 1;
			} else {
				while(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] != '\n' ) && is_word_whitespace( file_buffer[ selection[ i ].cursor - 1 ])){
					selection[ i ].cursor -= 1;
				}
				while(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] != '\n' ) && !is_word_whitespace( file_buffer[ selection[ i ].cursor - 1 ])){
					selection[ i ].cursor -= 1;
				}
			}
			i += 1;
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_pinned_line_next(){
	do {
		i32 i = 0;
		while( i < selection_count ){
			while(( selection[ i ].cursor < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor ] != '\n' )){
				selection[ i ].cursor += 1;
			}
			if( selection[ i ].cursor < file_count - 1 ){
				selection[ i ].cursor += 1;
			}
			i += 1;
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_pinned_line_prev(){
	do {
		i32 i = 0;
		while( i < selection_count ){
			while(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] != '\n' )){
				selection[ i ].cursor -= 1;
			}
			if( selection[ i ].cursor > 0 ){
				selection[ i ].cursor -= 1;
			}
			while(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] != '\n' )){
				selection[ i ].cursor -= 1;
			}
			i += 1;
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_pinned_para_next(){
	do {
		i32 i = 0;
		while( i < selection_count ){
			while( selection[ i ].cursor < file_count - 1 ){
				if(( file_buffer[ selection[ i ].cursor ] == '\n' ) && ( selection[ i ].cursor + 1 < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor + 1 ] == '\n' )){
					while(( selection[ i ].cursor < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor ] == '\n' )){
						selection[ i ].cursor += 1;
					}
					break;
				}
				selection[ i ].cursor += 1;
			}
			i += 1;
		}
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_move_pinned_para_prev(){
	do {
		i32 i = 0;
		while( i < selection_count ){
			while(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] == '\n' )){
				selection[ i ].cursor -= 1;
			}
			while( selection[ i ].cursor > 0 ){
				if(( file_buffer[ selection[ i ].cursor - 1 ] == '\n' ) && ( selection[ i ].cursor - 1 > 0 ) && ( file_buffer[ selection[ i ].cursor - 2 ] == '\n' )){
					break;
				}
				selection[ i ].cursor -= 1;
			}
			i += 1;
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

void command_move_pinned_find_next(){
	mode = append_find_next_mode;
}

void command_move_pinned_find_prev(){
	mode = append_find_prev_mode;
}

void command_move_line_end(){
	command_count = 0;
	i32 i = 0;
	while( i < selection_count ){
		selection[ i ].anchor = selection[ i ].cursor;
		while(( selection[ i ].cursor < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor ] != '\n' )){
			selection[ i ].cursor += 1;
		}
		i += 1;
	}
}

void command_move_line_start(){
	command_count = 0;
	i32 i = 0;
	while( i < selection_count ){
		selection[ i ].anchor = selection[ i ].cursor;
		while(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] != '\n' )){
			selection[ i ].cursor -= 1;
		}
		i += 1;
	}
}

void command_move_file_start(){
	command_count = 0;
	delete_all_non_primary_selections();
	selection[ 0 ].anchor = selection[ 0 ].cursor;
	selection[ 0 ].cursor = 0;
}

void command_move_file_end(){
	command_count = 0;
	delete_all_non_primary_selections();
	selection[ 0 ].anchor = selection[ 0 ].cursor;
	selection[ 0 ].cursor = file_count - 1;
}

void command_move_pinned_line_end(){
	command_count = 0;
	i32 i = 0;
	while( i < selection_count ){
		while(( selection[ i ].cursor < file_count - 1 ) && ( file_buffer[ selection[ i ].cursor ] != '\n' )){
			selection[ i ].cursor += 1;
		}
		i += 1;
	}
}

void command_move_pinned_line_start(){
	command_count = 0;
	i32 i = 0;
	while( i < selection_count ){
		while(( selection[ i ].cursor > 0 ) && ( file_buffer[ selection[ i ].cursor - 1 ] != '\n' )){
			selection[ i ].cursor -= 1;
		}
		i += 1;
	}
}

void command_move_pinned_file_start(){
	command_count = 0;
	delete_all_non_primary_selections();
	selection[ 0 ].cursor = 0;
}

void command_move_pinned_file_end(){
	command_count = 0;
	delete_all_non_primary_selections();
	selection[ 0 ].cursor = file_count - 1;
}

void command_select_inside_paren(){
/* Starting at the cursor, move the cursor back until it hits a '(' and move the anchor foward until it hits a ')'. */
	do {
		select_inside( "(", 1, ")", 1 );
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_select_inside_bracket(){
/* Starting at the cursor, move the cursor back until it hits a '[' and move the anchor foward until it hits a ']'. */
	do {
		select_inside( "[", 1, "]", 1 );
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_select_inside_curly(){
/* Starting at the cursor, move the cursor back until it hits a '{' and move the anchor foward until it hits a '}'. */
	do {
		select_inside( "{", 1, "}", 1 );
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_select_inside_double_quote(){
/* Starting at the cursor, move the cursor back until it hits a '"' and move the anchor foward until it hits a '"'. */
	do {
		select_inside( "\"", 1, "\"", 1 );
		command_count -= 1;
	} while( command_count > 0 );
	command_count = 0;
}

void command_swap_anchor_cursor(){
	command_count = 0;
	i32 i = 0;
	while( i < selection_count ){
		i32 tmp = selection[ i ].anchor;
		selection[ i ].anchor = selection[ i ].cursor;
		selection[ i ].cursor = tmp;
		i += 1;
	}
}

void command_select_entire_file(){
	command_count = 0;
	delete_all_non_primary_selections();
	selection[ 0 ].cursor = 0;
	selection[ 0 ].anchor = file_count - 1;
}

void command_count_goto(){
/*
  Move the primary selection to the start of line 'command_count'.
  Removes all other selections.
  Lines start at one.
  If command_count is 0, goto the first line.
  If command_count is greater then the number of lines in the file, goto the last character.
*/
	i64 index = 0;
	while(( index < file_count - 1 ) && ( command_count > 1 )){
		if( file_buffer[ index ] == '\n' ){
			command_count -= 1;
		}
		index += 1;
	}
	command_count = 0;
	delete_all_non_primary_selections();
	selection[ 0 ].cursor = index;
	selection[ 0 ].anchor = index;
}

void command_count_1(){
/* Add digit '1' to command_count. */
	command_count *= 10;
	command_count += 1;
}

void command_count_2(){
/* Add digit '2' to command_count. */
	command_count *= 10;
	command_count += 2;
}

void command_count_3(){
/* Add digit '3' to command_count. */
	command_count *= 10;
	command_count += 3;
}

void command_count_4(){
/* Add digit '4' to command_count. */
	command_count *= 10;
	command_count += 4;
}

void command_count_5(){
/* Add digit '5' to command_count. */
	command_count *= 10;
	command_count += 5;
}

void command_count_6(){
/* Add digit '6' to command_count. */
	command_count *= 10;
	command_count += 6;
}

void command_count_7(){
/* Add digit '7' to command_count. */
	command_count *= 10;
	command_count += 7;
}

void command_count_8(){
/* Add digit '8' to command_count. */
	command_count *= 10;
	command_count += 8;
}

void command_count_9(){
/* Add digit '9' to command_count. */
	command_count *= 10;
	command_count += 9;
}

void command_count_0(){
/* Add digit '0' to command_count. */
	command_count *= 10;
	command_count += 0;
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
	char buffer[ 128 ];
	i64 buffer_count = snprintf( buffer, 128, "%ld/%ld  ", primary_selection_index + 1, selection_count );
	frame_append( buffer, buffer_count );
}

void bar_line_number(){
	char buffer[ 128 ];
	i64 buffer_count = snprintf( buffer, 128, "%ld/%ld  ", line_number( file_buffer, file_count, selection[ primary_selection_index ].cursor ), line_number( file_buffer, file_count, file_count - 1 ));
	frame_append( buffer, buffer_count );
}

void bar_line_depth(){
	char buffer[ 128 ];
	i64 buffer_count = snprintf( buffer, 128, "%ld/%ld  ", line_depth( file_buffer, file_count, selection[ primary_selection_index ].cursor ), line_length( file_buffer, file_count, selection[ primary_selection_index ].cursor ));
	frame_append( buffer, buffer_count );
}

void bar_command_count(){
	if( command_count > 0 ){
		char buffer[ 128 ] = { '\0' };
		i64 buffer_count = snprintf( buffer, 128, "%ld  ", command_count );
		frame_append( buffer, buffer_count );
	}
}

void bar_search_string(){
	if( search_count > 0 ){
		char buffer[ 128 ] = { '\0' };
		i64 buffer_count = snprintf( buffer, 128, "\"%.*s\"  ", (i32) search_count, search_buffer );
		frame_append( buffer, buffer_count );
	}
}

i64 input_validate_next( u8* buffer, i64 count, i64* index ){
/*
  Move foward one utf8 codepoint, or ansi escape sequence.
  If the codepoint is cut in half, return how many bytes of this codepoint are at the end of the buffer.
  This function must handle any input that is valid in any part of the program, further culling should be done outside this function.
  This does not cull all invalid utf8, it culls all input Lute can not handle.
*/
	assert( buffer != NULL );
	assert( index != NULL );
	assert( count > *index );
	assert( count > 0 );
	assert( *index >= 0 );
	i64 clip = 0;
	u8 c = buffer[ *index ];
	if( c == 0x1b /* escape */ ){
		if( *index + 1 < count ){
			u8 c = buffer[ *index + 1 ];
			if(( *index + 2 < count ) && ( c == '[' )){
				u8 c = buffer[ *index + 2 ];
				if(( c >= 'A' ) && ( c <= 'H' )){
/* arrows / home / end */
					*index += 3;
				} else if(( *index + 3 < count ) && (( c == '3' ) || ( c == '5' ) || ( c == '6' ))){
/* page up / page down / delete */
					u8 c = buffer[ *index + 3 ];
					if(( *index + 3 < count ) && ( c == ';' )){
						u8 c = buffer[ *index + 4 ];
						if(( *index + 4 < count ) && ( c >= '2') && ( c <= '8' )){
							u8 c = buffer[ *index + 5 ];
							if(( *index + 5 < count ) && ( c == '~' )){
								*index += 6;
							} else {
								*index += 2;
							}
						} else {
							*index += 2;
						}
					} else if(( *index + 3 < count ) && ( c == '~' )){
						*index += 4;
					} else {
						*index += 2;
					}
				} else if(( *index + 3 < count ) && ( c == '1' )){
/* mod arrows / home / end */
					u8 c = buffer[ *index + 3 ];
					if(( *index + 3 < count ) && ( c == ';' )){
						u8 c = buffer[ *index + 4 ];
						if(( *index + 4 < count ) && ( c >= '2') && ( c <= '8' )){
							u8 c = buffer[ *index + 5 ];
							if(( *index + 5 < count ) && ( c >= 'A' ) && ( c <= 'H' )){
								*index += 6;
							} else {
								*index += 2;
							}
						} else {
							*index += 2;
						}
					} else {
						*index += 2;
					}
				} else {
					*index += 2;
				}
			} else {
				*index += 2;
			}
		} else {
			*index += 1;
		}
	} else if( c < 0x80 ){
/* ascii */
		*index += 1;
	} else if(( c >= 0xc0 ) && ( c < 0xe0 )){
/* two byte unicode */
		if( *index + 1 < count ){
			u8 c = buffer[ *index + 1 ];
			if( c >= 0x80 ){
				*index += 2;
			} else {
				error( "Input invalid utf8" );
			}
		} else {
			clip = 1;
		}
	} else if(( c >= 0xe0 ) && ( c < 0xf0 )){
/* three byte unicode */
		if( *index + 1 < count ){
			u8 c = buffer[ *index + 1 ];
			if( c >= 0x80 ){
				if( *index + 2 < count ){
					u8 c = buffer[ *index + 2 ];
					if( c >= 0x80 ){
						*index += 3;
					} else {
						error( "Input invalid utf8" );
					}
				} else {
					clip = 2;
				}
			} else {
				error( "Input invalid utf8" );
			}
		} else {
			clip = 1;
		}
	} else if(( c >= 0xf0 ) && ( c < 0xf8 )){
/* four byte unicode */
		if( *index + 1 < count ){
			u8 c = buffer[ *index + 1 ];
			if( c >= 0x80 ){
				if( *index + 2 < count ){
					u8 c = buffer[ *index + 2 ];
					if( c >= 0x80 ){
						if( *index + 3 < count ){
							u8 c = buffer[ *index + 3 ];
							if( c >= 0x80 ){
								*index += 4;
							} else {
								error( "Input invalid utf8" );
							}
						} else {
							clip = 3;
						}
					} else {
						error( "Input invalid utf8" );
					}
				} else {
					clip = 2;
				}
			} else {
				error( "Input invalid utf8" );
			}
		} else {
			clip = 1;
		}
	} else {
		error( "Input invalid utf8" );
	}
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
	if( argc != 2 ){
		error( "Usage: lute <filename>" );
	}
	file_name = argv[ 1 ];
	terminal_read_file();
	i64 file_index = 0;
	while( file_index < file_count ){
		i64 start_index = file_index;
		i64 clip = input_validate_next( (u8*) file_buffer, file_count, &file_index );
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
	terminal_setup();
	while( 1 ){
/*
  Main program loop
   +-> get window size
   |    v
   |   draw frame
   |    v
   |   get and process input
   |    v
   +-< clip overlaped selections
*/
		terminal_get_dimensions();
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
				i32 i = 0;
				while( i < screen_rows / 2 ){
					if( draw_index == 0 ){
						empty_lines_above = ( screen_rows / 2 ) - i;
						break;
					}
					draw_index -= 1;
					while(( draw_index > 0 ) && ( file_buffer[ draw_index - 1 ] != '\n' )){
						draw_index -= 1;
					}
					i += 1;
				}
			}
			if( bar_possition == top_bar ){
				i32 i = 0;
				while( i < (i32)( sizeof( bar ) / sizeof( bar_item ))){
					bar[ i ].function();
					i += 1;
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
					i32 i = 0;
					while( i < selection_count ){
						if( selection[ i ].cursor < draw_index ){
							highlight = !highlight;
						}
						if( selection[ i ].anchor < draw_index ){
							highlight = !highlight;
						}
						if( selection[ i ].anchor >= draw_index || selection[ i ].cursor >= draw_index ){
							break;
						}
						i += 1;
					}
				}
			}
			i32 i = 0;
			while( i < (( bar_possition == no_bar ) ? screen_rows : screen_rows - 1 ) ){
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
						i32 i = primary_line_number + screen_rows / 2 - 1;
						while( i > 0 ){
							filled_cols += 1;
							i /= 10;
						}
						char buffer[ 128 ] = { '\0' };
						if( draw_line_numbers == regular_line_numbers ){
							line_print = primary_line_number - screen_rows / 2 + i;
						} else if( draw_line_numbers == relitive_line_numbers ){
							line_print = ( i == screen_rows / 2 ) ? primary_line_number : abs( (i32)( screen_rows / 2 - i ));
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
						i32 i = 0;
						while( i < selection_count ){
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
							i += 1;
						}
						if( file_buffer[ draw_index ] == '\n' ){
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
						} else if(( file_buffer[ draw_index ] & 0x80 ) == 0 ){  /* ascii */
							filled_cols += 1;
							col_bytes = 1;
						} else if(( file_buffer[ draw_index ] & 0xe0 ) == 0xc0 ){  /* two byte unicode */
							filled_cols += 1;
							col_bytes = 2;
						} else if(( file_buffer[ draw_index ] & 0xf0 ) == 0xe0 ){  /* three byte unicode */
							filled_cols += 1;
							col_bytes = 3;
						} else if(( file_buffer[ draw_index ] & 0xf8 ) == 0xf0 ){  /* four byte unicode */
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
					i64 line_end_index = draw_index;
					while( draw_index < file_count - 1 && file_buffer[ draw_index ] != '\n' ){
						draw_index += 1;
					}
					if( line_end_index != draw_index ){
						i32 i = 0;
						while( i < selection_count ){
							if(( selection[ i ].cursor <= draw_index ) && ( selection[ i ].cursor >= line_end_index )){
								if( i == primary_selection_index ){
									highlight = ( highlight ) ? 0 : 2;
								} else {
									highlight = !highlight;
								}
							}
							if(( selection[ i ].anchor <= draw_index ) && ( selection[ i ].anchor >= line_end_index )){
								if( i == primary_selection_index ){
									highlight = ( highlight ) ? 0 : 2;
								} else {
									highlight = !highlight;
								}
							}
							i += 1;
						}
					}
					draw_index += 1;
				}
				i += 1;
			}
			if( bar_possition == bottom_bar ){
				frame_append( "\n", 1 );
				i32 i = 0;
				while( i < (i32)( sizeof( bar ) / sizeof( bar_item ))){
					bar[ i ].function();
					i += 1;
				}
			}
			terminal_draw_frame();
			frame_count = 0;
		}
		{
/*
  Wait for input then process it
*/
			i64 input_index = 0;
			i64 input_clip = 0;
			assert( input_count >= 0 );
			input_count = read( STDIN_FILENO, &input_buffer[ input_count ], max_input_bytes - input_count ) + input_count;  /* will block until input */
			assert( input_count > 0 );
			assert( input_count <= max_input_bytes );
			while( input_index < input_count ){
				if( mode == command_mode ){
					i64 start_index = input_index;
					input_clip = input_validate_next( (u8*) input_buffer, input_count, &input_index );
					if( input_clip != 0 ){
						break;
					}
					assert( start_index < input_index );
					i64 key_bytes = input_index - start_index;
					assert( key_bytes > 0 );
					i32 i = 0;
					while( i < (i32)( sizeof( command ) / sizeof( keybind )) ){
						if(( key_bytes == (i64) strlen( command[ i ].key )) && ( memcmp( &input_buffer[ start_index ], command[ i ].key, key_bytes ) == 0 )){
							command[ i ].function();
							clip_selection_overlap();
						}
						i += 1;
					}
					if(( key_bytes == 1 ) && ( input_buffer[ start_index ] == 0x1b )){
						command_count = 0;
						warning = NULL;
					}
				} else if( mode == edit_mode ){
					i64 start_index = input_index;
					i64 insert_bytes = 0;
					do {
						assert( start_index + insert_bytes == input_index );
						i64 loop_index = input_index;
						input_clip = input_validate_next( (u8*) input_buffer, input_count, &input_index );
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
							if( insert_bytes > 0 ){
								selection_insert( &input_buffer[ start_index ], insert_bytes );
							}
							selection_delete_backspace();
							insert_bytes = 0;
							break;
						} else if( is_ascii_unprintable( input_buffer, loop_index, input_index - loop_index )){
							warning = "Can only input text";
							break;
						}
						insert_bytes += input_index - loop_index;
					} while( input_index < input_count );
					if( insert_bytes > 0 ){
						selection_insert( &input_buffer[ start_index ], insert_bytes );
					}
					if( input_clip != 0 ){
						break;
					}
				} else if( mode == search_mode ){
					i64 start_index = input_index;
					input_clip = input_validate_next( (u8*) input_buffer, input_count, &input_index );
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
						i32 i = 0;
						while( i < input_index - start_index ){
							search_buffer[ search_count ] = input_buffer[ start_index + i ];
							search_count += 1;
							i += 1;
						}
					}
				} else if(( mode == find_next_mode ) || ( mode == find_prev_mode ) || ( mode == append_find_next_mode ) || ( mode == append_find_prev_mode )){
					i64 start_index = input_index;
					input_clip = input_validate_next( (u8*) input_buffer, input_count, &input_index );
					if( input_clip != 0 ){
						break;
					}
					assert( input_index > start_index );
					i64 key_bytes = input_index - start_index;
					if( is_ansi_escape( input_buffer, start_index, key_bytes )){
						warning = "Can only input text";
					} else if( input_buffer[ start_index ] == 0x1b /* escape */ ){
						/* do nothing */
					} else if( is_ascii_unprintable( input_buffer, start_index, key_bytes )){
						warning = "Can only input text";
					} else {
						do {
							i32 i = 0;
							while( i < selection_count ){
								if( mode == find_next_mode ){
									i64 j = selection[ i ].cursor + 1;
									while( j < file_count - 1 ){
										if( strncmp( &file_buffer[ j ], &input_buffer[ start_index ], key_bytes ) == 0 ){
											selection[ i ].anchor = selection[ i ].cursor;
											selection[ i ].cursor = j;
											break;
										};
										j += 1;
									}
								} else if( mode == find_prev_mode ){
									i64 j = selection[ i ].cursor - 1;
									while( j >= 0 ){
										if( strncmp( &file_buffer[ j ], &input_buffer[ start_index ], key_bytes ) == 0 ){
											selection[ i ].anchor = selection[ i ].cursor;
											selection[ i ].cursor = j;
											break;
										};
										j -= 1;
									}
								} else if( mode == append_find_next_mode ){
									i64 j = selection[ i ].cursor + 1;
									while( j < file_count - 1 ){
										if( strncmp( &file_buffer[ j ], &input_buffer[ start_index ], key_bytes ) == 0 ){
											selection[ i ].cursor = j;
											break;
										};
										j += 1;
									}
								} else if( mode == append_find_prev_mode ){
									i64 j = selection[ i ].cursor - 1;
									while( j >= 0 ){
										if( strncmp( &file_buffer[ j ], &input_buffer[ start_index ], key_bytes ) == 0 ){
											selection[ i ].cursor = j;
											break;
										};
										j -= 1;
									}
								}
								i += 1;
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
			assert( input_index == input_count );
			assert( input_index - input_clip > 0 );
			memmove( input_buffer, &input_buffer[ input_count - input_clip ], input_clip );
			input_count = input_clip;
		}
		clip_selection_overlap();
	}
	error( "Somehow broke out of main loop" );
}
