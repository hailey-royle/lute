#define max_file_size         0x4000000    //  64 Mib
#define max_clipboard_size    0x4000000    //  64 Mib
#define max_edit_size         0x4000000    //  64 Mib
#define max_frame_size        0x10000      //  64 Kib
#define max_input_size        0x1000       //   4 Kib
#define max_search_size       0x400        //   1 Kib

#define max_edit_count        1024 * 1024
#define max_selection_count   1024

#define primary_cursor_highlight_start ( ansi_inverse_start )
#define primary_cursor_highlight_end ( ansi_inverse_end )

#define cursor_highlight_start ( ansi_foreground_cyan ansi_inverse_start )
#define cursor_highlight_end ( ansi_inverse_end ansi_foreground_default )

#define primary_selection_highlight_start ( ansi_background_red )
#define primary_selection_highlight_end ( ansi_background_default )

#define selection_highlight_start ( ansi_background_green )
#define selection_highlight_end ( ansi_background_default )

i32 tab_width = 8;

bar_item bar[] = {
//	{ function },
	{ bar_file_name },
	{ bar_mode },
	{ bar_selection },
	{ bar_line_number },
	{ bar_line_depth },
	{ bar_command_count },
	{ bar_search_string },
};

keybind command[] = {
//	{ key, function },
	{ "Q", command_quit },
	{ "q", command_write_quit },
	{ "w", command_write },
	{ " ", command_edit_mode },
	{ "\n", command_edit_newline },
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
	{ "s", command_split },
	{ "S", command_split_newline },
	{ ";", command_split_collapse },
	{ ".", command_split_next },
	{ ",", command_split_prev },
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
	{ "L", command_move_append_char_next },
	{ "H", command_move_append_char_prev },
	{ "E", command_move_append_word_next },
	{ "B", command_move_append_word_prev },
	{ "J", command_move_append_line_next },
	{ "K", command_move_append_line_prev },
	{ "N", command_move_append_para_next },
	{ "M", command_move_append_para_prev },
	{ "F", command_move_append_find_next },
	{ "V", command_move_append_find_prev },
	{ "x", command_move_line_end },
	{ "z", command_move_line_start },
	{ "T", command_move_file_end },
	{ "t", command_move_file_start },
	{ "(", command_select_inside_paren },
	{ ")", command_select_inside_paren },
	{ "[", command_select_inside_bracket },
	{ "]", command_select_inside_bracket },
	{ "{", command_select_inside_curly },
	{ "}", command_select_inside_curly },
	{ "\"", command_select_inside_double_quote },
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
