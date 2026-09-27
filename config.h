#define max_file_size         0x4000000    //  64 Mib
#define max_clipboard_size    0x4000000    //  64 Mib
#define max_edit_size         0x4000000    //  64 Mib
#define max_frame_size        0x10000      //  64 Kib
#define max_input_size        0x400        //   1 Kib

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
};

keybind command[] = {
//	{ key, function },
	{ "Q", command_quit },
	{ "q", command_write_quit },
	{ "w", command_write },
	{ " ", command_edit_mode },
	{ "u", command_undo },
	{ "U", command_redo },
	{ "y", command_copy },
	{ "d", command_delete },
	{ "c", command_change },
	{ "p", command_paste },
	{ "r", command_replace },
	{ "l", command_move_char_next },
	{ "h", command_move_char_prev },
	{ "e", command_move_word_next },
	{ "b", command_move_word_prev },
	{ "j", command_move_line_next },
	{ "k", command_move_line_prev },
	{ "n", command_move_para_next },
	{ "m", command_move_para_prev },
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
};
