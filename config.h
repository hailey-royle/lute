/*
*/

/*
  Maximum usable memory per catagorie.
  Can be as large as you have avalible memory.
  Lute will exit if any max is reached.
*/
/* max_file_bytes should be at least as big as the largest possible readable file */
#define max_file_bytes         0x4000000    /* 64 Mib */

/* max_clipboard_bytes should be at least as big as the largest possible editable file */
#define max_clipboard_bytes    0x4000000    /* 64 Mib */

/* max_edit_bytes should be at least as big as the largest possible editable file */
#define max_edit_bytes         0x4000000    /* 64 Mib */

/* max_frame_bytes should be at least as big as your screen resolution / 64 */
#define max_frame_bytes        0x20000      /* 128 Kib */

/* max_search_bytes should be at least as big as the longest string you would ever search for in the file */
#define max_search_bytes       0x1000       /* 4 Kib */


/*
  max_input_bytes can be at most 0xff because of Linux pasting weirdenss.
  (Note: pasting in more then 4 Kib is supported.)
*/
#define max_input_bytes        0xfff        /* 4 Kib - 1 */

/* Maximum possible edit history count, per selection. */
#define max_edit_count        1024 * 1024

/* Maximum possible selection count. */
#define max_selection_count   1024

/*
  What will happend at the start/end of a selection/cursor.
  Full list in lute.c.
  If you dont know what you are doing, it is recomended to only change the color.
  (red, green, blue, yellow, magenta, cyan)
*/
#define primary_cursor_highlight_start ( ansi_inverse_start )
#define primary_cursor_highlight_end ( ansi_inverse_end )
#define primary_selection_highlight_start ( ansi_background_red )
#define primary_selection_highlight_end ( ansi_background_default )

#define cursor_highlight_start ( ansi_foreground_cyan ansi_inverse_start )
#define cursor_highlight_end ( ansi_inverse_end ansi_foreground_default )
#define selection_highlight_start ( ansi_background_green )
#define selection_highlight_end ( ansi_background_default )

/* what is input when pressing the tab key. */
char* tab_chars = "\t";

/* length of '\t' in the file. (Max 16)*/
i32 tab_width = 8;

/* options for displaying line numbers. */
#define no_line_numbers 0
#define regular_line_numbers 1
#define relitive_line_numbers 2

i8 draw_line_numbers = relitive_line_numbers;

/* options for displaying the status bar. */
#define no_bar 0
#define bottom_bar 1
#define top_bar 2

i8 bar_position = top_bar;

/*
  The order bar items will be drawn.
  If 'bar_position' is 'no_bar', this is ignored.
  The bar starts aligning items to the left.
  After calling 'bar_mode_center', items will be aligned to the center.
  After calling 'bar_mode_right', items will be aligned to the right.
  'bar_mode_center' must come before 'bar_mode_right', unless there is nothing to be centered, then 'bar_mode_center' can be skipped.
*/
bar_item bar[] = {
/*	{ function }, */
	{ bar_warning },
	{ bar_editor_mode },
	{ bar_command_count },
	{ bar_search_string },
	{ bar_draw_mode_center },
	{ bar_file_name },
	{ bar_draw_mode_right },
	{ bar_selection },
	{ bar_line_number },
	{ bar_line_depth },
};

/*
  What keys map to what function.
  All command functions start with 'command_'.
  See key_*key* #defines in lute.c for more advanced options.
  Use keybind.c for keybinds not provided in lute.c or unicode keybinds.
  (Note: some terminals have different codes then the defaults provided in lute.c, when in doubt, use keybind.c)
*/
keybind command[] = {
/*	{ key, function }, */
	{ "Q", command_quit },
	{ "q", command_write_quit },
	{ "w", command_write },
	{ "i", command_edit_mode },
	{ "o", command_edit_newline },
	{ ">", command_indent },
	{ "<", command_deindent },
	{ "u", command_undo },
	{ "U", command_redo },
	{ "p", command_paste },
	{ "y", command_copy },
	{ "d", command_delete },
	{ "c", command_change },
	{ "r", command_replace },
	{ "Y", command_line_copy },
	{ "D", command_line_delete },
	{ "C", command_line_change },
	{ "R", command_line_replace },
	{ "s", command_search_input },
	{ "/", command_search_next_primary },
	{ "?", command_search_prev_primary },
	{ ":", command_search_file_primary },
	{ "S", command_split_newline },
	{ ";", command_split_collapse },
	{ ".", command_next_selection },
	{ ",", command_prev_selection },
	{ "l", command_move_char_next },
	{ "h", command_move_char_prev },
	{ "e", command_move_word_next },
	{ "b", command_move_word_prev },
	{ "j", command_move_line_next },
	{ "k", command_move_line_prev },
	{ "n", command_move_para_next },
	{ "m", command_move_para_prev },
	{ "f", command_move_find_next },
	{ "v", command_move_find_prev },
	{ "L", command_move_pinned_char_next },
	{ "H", command_move_pinned_char_prev },
	{ "E", command_move_pinned_word_next },
	{ "B", command_move_pinned_word_prev },
	{ "J", command_move_pinned_line_next },
	{ "K", command_move_pinned_line_prev },
	{ "N", command_move_pinned_para_next },
	{ "M", command_move_pinned_para_prev },
	{ "F", command_move_pinned_find_next },
	{ "V", command_move_pinned_find_prev },
	{ "x", command_move_line_end },
	{ "z", command_move_line_start },
	{ "x", command_move_pinned_line_end },
	{ "z", command_move_pinned_line_start },
	{ "T", command_move_file_end },
	{ "t", command_move_file_start },
	{ "(", command_select_inside_paren },
	{ ")", command_select_inside_paren },
	{ "[", command_select_inside_bracket },
	{ "]", command_select_inside_bracket },
	{ "{", command_select_inside_curly },
	{ "}", command_select_inside_curly },
	{ "\"", command_select_inside_double_quote },
	{ "'", command_select_inside_single_quote },
	{ "a", command_swap_anchor_cursor },
	{ "A", command_select_entire_file },
	{ "g", command_count_goto },
	{ "1", command_count_1 },
	{ "2", command_count_2 },
	{ "3", command_count_3 },
	{ "4", command_count_4 },
	{ "5", command_count_5 },
	{ "6", command_count_6 },
	{ "7", command_count_7 },
	{ "8", command_count_8 },
	{ "9", command_count_9 },
	{ "0", command_count_0 },
};
