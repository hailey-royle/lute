#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>

#include "string.h"
#include "assert.h"
#include "tui.h"

enum Mode {
	NORMAL_MODE,
	EDIT_MODE,
	FIND_PREV_MODE,
	FIND_NEXT_MODE,
	SEARCH_MODE,
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

struct SelectionArray selection = { 0 };
struct EditArray edit = { 0 };
struct Screen screen = { 0 };
struct String file = { 0 };
struct String search = { 0 };
struct String bar_notice = { 0 };
char* file_name = NULL;
size_t command_count = 0;
enum Mode mode = NORMAL_MODE;
bool file_modified = false;
bool anchor_pinned = false;

void CommandEscapeNormal();
void CommandQuit();
void CommandWriteFileQuit();
void CommandWriteFile();
void CommandStandardEditMode();
void CommandNewlineEditMode();
void CommandFindPrevMode();
void CommandFindNextMode();
void CommandEditUndo();
void CommandEditRedo();
void CommandToggleAnchorPin();
void CommandSwapCursorAnchor();
void CommandMoveCharPrev();
void CommandMoveCharNext();
void CommandMoveLinePrev();
void CommandMoveLineNext();
void CommandMoveWordPrev();
void CommandMoveWordNext();
void CommandMoveParagraphPrev();
void CommandMoveParagraphNext();
void CommandMoveLineStart();
void CommandMoveLineEnd();
void CommandMoveFileStart();
void CommandMoveFileEnd();
void CommandMoveLineNumber();
void CommandSelectFile();
void CommandSelectInsideParen();
void CommandSelectInsideBracket();
void CommandSelectInsideCurly();
void CommandSelectInsideSingleQuote();
void CommandSelectInsideDoubleQuote();
void CommandCopySelection();
void CommandCopyLine();
void CommandDeleteSelection();
void CommandDeleteLine();
void CommandChangeSelection();
void CommandChangeLine();
void CommandPasteClipboard();
void CommandReplaceSelection();
void CommandReplaceLine();
void CommandSearchString();
void CommandSearchNewline();
void CommandRemoveOtherSelections();
void CommandRotateSelection();
void CommandDeleteAtLineStart();
void CommandInsertTabAtLineStart();
void CommandCount0();
void CommandCount1();
void CommandCount2();
void CommandCount3();
void CommandCount4();
void CommandCount5();
void CommandCount6();
void CommandCount7();
void CommandCount8();
void CommandCount9();

#include "config.h"

void LoadArgs( int argc, char** argv ){
	if( argc != 2 ){
		printf( "Usage: lute <filename>\n" );
		exit( 0 );
	}
	file_name = argv[ 1 ];
}

void DrawBar( struct String* print ){
	StringAppend( print, ERASE_LINE, sizeof( ERASE_LINE ));
	if( bar_notice.data != NULL ){
		StringAppend( print, bar_notice.data, bar_notice.len );
		return;
	}
	struct String bar = { 0 };
	StringAlloc( &bar, screen.rows );
	StringAppend( &bar, file_name, strlen( file_name ));
	if( file_modified == true ){
		StringAppend( &bar, "+", 1 );
	}
	StringAlloc( &bar, 64 );
	if( anchor_pinned == true ){
		bar.len += sprintf( &bar.data[ bar.len ], "  !%ld", selection.count );
	} else {
		bar.len += sprintf( &bar.data[ bar.len ], "  %ld", selection.count );
	}
	bar.len += sprintf( &bar.data[ bar.len ], ":%ld", StringGetLineNumber( &file, selection.data[ 0 ].cursor ));
	bar.len += sprintf( &bar.data[ bar.len ], ":%ld", StringGetLineDepth( &file, selection.data[ 0 ].cursor ));
	if( mode == EDIT_MODE ){
		StringAppend( &bar, "  ==EDIT==", 10 );
	} else if( mode == NORMAL_MODE ){
		StringAppend( &bar, "  =NORMAL=", 10 );
	} else if( mode == FIND_NEXT_MODE || mode == FIND_PREV_MODE ){
		StringAppend( &bar, "  ==FIND==", 10 );
	} else if( mode == SEARCH_MODE ){
		StringAppend( &bar, "  =SEARCH=", 10 );
		StringAppend( &bar, "  \"", 3 );
		if( search.data != NULL ){
			StringAppend( &bar, search.data, search.len );
		}
		StringAppend( &bar, "\"", 1 );
	} else {
		Unreachable();
	}
	StringAlloc( &bar, 32 );
	if( command_count > 0 ){
		bar.len += sprintf( &bar.data[ bar.len ], "  %ld", command_count );
	}
	size_t final_bar_length = ( screen.cols < bar.len ) ? screen.cols : bar.len;
	StringAppend( print, bar.data, final_bar_length );
	StringFree( &bar );
}

size_t DrawLine( struct String* print, size_t start_index, bool* highlight ){
	size_t real_end_index = StringSelectLineEnd( &file, start_index ) + 1;
	size_t cliped_end_index = real_end_index;
	if( real_end_index - start_index > screen.cols ){
		cliped_end_index = start_index + screen.cols;
	}
	if( *highlight == true ){
		StringAppend( print, HIGHLIGHT_START, sizeof( HIGHLIGHT_START ));
	}
	size_t tabs = 0;
	size_t unicode = 0;
	for( size_t i = start_index; i < cliped_end_index; ){
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
			tabs++;
			i++;
			if( real_end_index - start_index > screen.cols - ( 7 * tabs ) + unicode ){
				cliped_end_index = start_index + screen.cols - ( 7 * tabs ) + unicode;
			}
		} else {
			size_t i_step = StringUTF8Next( &file, i ) - i;
			StringAppend( print, &file.data[ i ], i_step );
			unicode += i_step - 1;
			i += i_step;
			if( real_end_index - start_index > screen.cols - ( 7 * tabs ) + unicode ){
				cliped_end_index = start_index + screen.cols - ( 7 * tabs ) + unicode;
			}
		}
		if( inverse_flag ){
			StringAppend( print, INVERSE_END, sizeof( INVERSE_END ));
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
	DrawBar( &print );
	for( size_t i = 1; i < screen.rows; i++ ){
		StringAppend( &print, "\r\n", 2 );
		StringAppend( &print, ERASE_LINE, sizeof( ERASE_LINE ));
		if( drawLine < 0 || drawIndex >= file.len ){
			StringAppend( &print, "~", 1 );
			drawLine++;
		} else {
			drawIndex = DrawLine( &print, drawIndex, &highlight );
			drawLine++;
		}
	}
	write( STDOUT_FILENO, print.data, print.len );
	StringFree( &print );
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
	command_count = 0;
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
	command_count = 0;
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
	mode = EDIT_MODE;
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
	file_modified = true;
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
	file_modified = true;
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
	file_modified = true;
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

void ProsessSearchSubstring(){
	if( search.len == 0 || selection.data[ 0 ].cursor == selection.data[ 0 ].anchor ){
		return;
	}
	size_t selection_min = ( selection.data[ 0 ].anchor < selection.data[ 0 ].cursor ) ? selection.data[ 0 ].anchor : selection.data[ 0 ].cursor;
	size_t selection_max = ( selection.data[ 0 ].anchor > selection.data[ 0 ].cursor ) ? selection.data[ 0 ].anchor : selection.data[ 0 ].cursor;
	selection_min = ( selection_min == 0 ) ? 0 : selection_min - 1;
	size_t new_selection_index = StringSelectSubStringNext( &file, selection_min, search.data, search.len );
	if( new_selection_index + search.len < selection_max ){
		for( size_t i = 1; selection.count > 1; ){
			SelectionFree( i );
		}
		if( selection.data[ 0 ].anchor > selection.data[ 0 ].cursor ){
			size_t tmp = selection.data[ 0 ].cursor;
			selection.data[ 0 ].cursor = selection.data[ 0 ].anchor;
			selection.data[ 0 ].anchor = tmp;
		}
		selection.data[ 0 ].anchor = new_selection_index;
		selection.data[ 0 ].cursor = new_selection_index + search.len;
		while( true ){
			new_selection_index = StringSelectSubStringNext( &file, new_selection_index, search.data, search.len );
			if( new_selection_index == file.len - 1 || new_selection_index + search.len > selection_max ){
				break;
			}
			SelectionNew( new_selection_index );
			selection.data[ selection.count - 1 ].cursor = new_selection_index + search.len;
		}
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

#define ProsessSelectionMoveKey( func, key ){ \
	if( command_count == 0 ){ \
		command_count = 1; \
	} \
	for( size_t i = 0; i < selection.count; i++ ){ \
		for( size_t j = 0; j < command_count; j++ ){ \
			if( !anchor_pinned ){ \
				selection.data[ i ].anchor = selection.data[ i ].cursor; \
			} \
			selection.data[ i ].cursor = func( &file, selection.data[ i ].cursor, key ); \
		} \
	} \
	command_count = 0; \
}

#define ProsessSelectionInside( left, right ){ \
	for( size_t i = 0; i < selection.count; i++ ){ \
		size_t lower_index = selection.data[ i ].cursor; \
		size_t upper_index = selection.data[ i ].cursor; \
		size_t j = ( file.data[ selection.data[ i ].cursor ] == right ) ? 1 : 0; \
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
		j = ( file.data[ selection.data[ i ].cursor ] == left ) ? 1 : 0; \
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
			selection.data[ i ].cursor = upper_index; \
			selection.data[ i ].anchor = lower_index + 1; \
		} \
	} \
}

void CommandEscapeNormal(){
	command_count = 0;
}

void CommandQuit(){
	exit( 0 );
}

void CommandWriteFileQuit(){
	command_count = 0;
	bool error = StringToFile( &file, file_name );
	if( error == true ){
		StringAppend( &bar_notice, "ERROR: Could not write to file.", 30 );
		return;
	}
	file_modified = false;
	exit( 0 );
}

void CommandWriteFile(){
	command_count = 0;
	bool error = StringToFile( &file, file_name );
	if( error == true ){
		StringAppend( &bar_notice, "ERROR: Could not write to file.", 30 );
		return;
	}
	file_modified = false;
}

void CommandStandardEditMode(){
	command_count = 0;
	EditNew();
	EditModeInit();
}

void CommandNewlineEditMode(){
	command_count = 0;
	ProsessSelectionMove( StringSelectLineEnd );
	EditNew();
	EditModeInit();
	ProsessEditInsert( "\n", 1 );
}

void CommandFindPrevMode(){
	mode = FIND_PREV_MODE;
}

void CommandFindNextMode(){
	mode = FIND_NEXT_MODE;
}

void CommandEditUndo(){
	EditUndo();
}

void CommandEditRedo(){
	EditRedo();
}

void CommandToggleAnchorPin(){
	command_count = 0;
	if( anchor_pinned ){
		anchor_pinned = false;
	} else {
		anchor_pinned = true;
	}
}

void CommandSwapCursorAnchor(){
	command_count = 0;
	for( size_t i = 0; i < selection.count; i++ ){
		size_t tmp = selection.data[ i ].cursor;
		selection.data[ i ].cursor = selection.data[ i ].anchor;
		selection.data[ i ].anchor = tmp;
	}
}

void CommandMoveCharPrev(){
	ProsessSelectionMove( StringSelectCharPrev );
}

void CommandMoveCharNext(){
	ProsessSelectionMove( StringSelectCharNext );
}

void CommandMoveLinePrev(){
	ProsessSelectionMove( StringSelectLinePrev );
}

void CommandMoveLineNext(){
	ProsessSelectionMove( StringSelectLineNext );
}

void CommandMoveWordPrev(){
	ProsessSelectionMove( StringSelectWordPrev );
}

void CommandMoveWordNext(){
	ProsessSelectionMove( StringSelectWordNext );
}

void CommandMoveParagraphPrev(){
	ProsessSelectionMove( StringSelectParagraphPrev );
}

void CommandMoveParagraphNext(){
	ProsessSelectionMove( StringSelectParagraphNext );
}

void CommandMoveLineStart(){
	ProsessSelectionMove( StringSelectLineStart );
}

void CommandMoveLineEnd(){
	ProsessSelectionMove( StringSelectLineEnd );
}

void CommandMoveFileEnd(){
	command_count = 0;
	for( size_t i = 0; i < selection.count; i++ ){
		if( !anchor_pinned ){
			selection.data[ i ].anchor = file.len - 1;
		}
		selection.data[ i ].cursor = file.len - 1;
	}
}

void CommandMoveFileStart(){
	command_count = 0;
	for( size_t i = 0; i < selection.count; i++ ){
		if( !anchor_pinned ){
			selection.data[ i ].anchor = 0;
		}
		selection.data[ i ].cursor = 0;
	}
}

void CommandMoveLineNumber(){
	size_t line_index = StringSelectLineNumber( &file, command_count );
	for( size_t i = 0; i < selection.count; i++ ){
		selection.data[ i ].cursor = line_index;
		selection.data[ i ].anchor = selection.data[ i ].cursor;
	}
	command_count = 0;
}

void CommandSelectFile(){
	command_count = 0;
	for( size_t i = 0; i < selection.count; i++ ){
		selection.data[ i ].cursor = 0;
		selection.data[ i ].anchor = file.len - 1;
	}
}

void CommandSelectInsideParen(){
	command_count = 0;
	ProsessSelectionInside( '(', ')' );
}

void CommandSelectInsideBracket(){
	command_count = 0;
	ProsessSelectionInside( '[', ']' );
}

void CommandSelectInsideCurly(){
	command_count = 0;
	ProsessSelectionInside( '{', '}' );
}

void CommandSelectInsideSingleQuote(){
	command_count = 0;
	ProsessSelectionInside( '\'', '\'' );
}

void CommandSelectInsideDoubleQuote(){
	command_count = 0;
	ProsessSelectionInside( '"', '"' );
}

void CommandCopySelection(){
	command_count = 0;
	CopySelection();
}

void CommandCopyLine(){
	command_count = 0;
	SelectCursorLine();
	CopySelection();
}

void CommandDeleteSelection(){
	command_count = 0;
	EditNew();
	CopySelection();
	DeleteSelection();
}

void CommandDeleteLine(){
	command_count = 0;
	SelectCursorLine();
	EditNew();
	CopySelection();
	DeleteSelection();
}

void CommandChangeSelection(){
	command_count = 0;
	EditNew();
	CopySelection();
	DeleteSelection();
	EditModeInit();
}

void CommandChangeLine(){
	command_count = 0;
	SelectCursorLine();
	EditNew();
	CopySelection();
	DeleteSelection();
	EditModeInit();
}

void CommandPasteClipboard(){
	command_count = 0;
	EditNew();
	PasteSelection();
}

void CommandReplaceSelection(){
	command_count = 0;
	EditNew();
	DeleteSelection();
	PasteSelection();
}

void CommandReplaceLine(){
	command_count = 0;
	SelectCursorLine();
	EditNew();
	DeleteSelection();
	PasteSelection();
}

void CommandSearchString(){
	command_count = 0;
	mode = SEARCH_MODE;
}

void CommandSearchNewline(){
	command_count = 0;
	char newline = '\n';
	StringAppend( &search, &newline, 1 );
	ProsessSearchSubstring();
	StringFree( &search );
}

void CommandRemoveOtherSelections(){
	command_count = 0;
	for( size_t i = 1; selection.count > 1; ){
		SelectionFree( i );
	}
}

void CommandRotateSelection(){
	if( command_count == 0 ){
		command_count = 1;
	}
	for( size_t i = 0; i < command_count; i++ ){
		struct Selection tmp = selection.data[ 0 ];
		for( size_t j = 1; j < selection.count; j++ ){
			selection.data[ j - 1 ].cursor = selection.data[ j ].cursor;
			selection.data[ j - 1 ].anchor = selection.data[ j ].anchor;
			selection.data[ j - 1 ].clipboard.data = selection.data[ j ].clipboard.data;
			selection.data[ j - 1 ].clipboard.len = selection.data[ j ].clipboard.len;
			selection.data[ j - 1 ].clipboard.cap = selection.data[ j ].clipboard.cap;
		}
		selection.data[ selection.count - 1 ] = tmp;
	}
	command_count = 0;
}

void CommandDeleteAtLineStart(){
	command_count = 0;
	ProsessSelectionMove( StringSelectLineStart );
	ProsessSelectionMove( StringSelectCharNext );
	EditNew();
	for( size_t i = 0; i < selection.count; i++ ){
		edit.data[ edit.undo_count - 1 ].data[ i ].index = selection.data[ i ].cursor;
	}
	ProsessEditDelete( 1 );
}

void CommandInsertTabAtLineStart(){
	command_count = 0;
	ProsessSelectionMove( StringSelectLineStart );
	EditNew();
	for( size_t i = 0; i < selection.count; i++ ){
		edit.data[ edit.undo_count - 1 ].data[ i ].index = selection.data[ i ].cursor;
	}
	ProsessEditInsert( "\t", 1 );
	ProsessSelectionMove( StringSelectLineStart );
}

void CommandCount0(){
	command_count *= 10;
	command_count += 0;
}

void CommandCount1(){
	command_count *= 10;
	command_count += 1;
}

void CommandCount2(){
	command_count *= 10;
	command_count += 2;
}

void CommandCount3(){
	command_count *= 10;
	command_count += 3;
}

void CommandCount4(){
	command_count *= 10;
	command_count += 4;
}

void CommandCount5(){
	command_count *= 10;
	command_count += 5;
}

void CommandCount6(){
	command_count *= 10;
	command_count += 6;
}

void CommandCount7(){
	command_count *= 10;
	command_count += 7;
}

void CommandCount8(){
	command_count *= 10;
	command_count += 8;
}

void CommandCount9(){
	command_count *= 10;
	command_count += 9;
}

void ProsessCommand( int32_t key ){
	for( size_t i = 0; i < sizeof( command ) / sizeof( struct Key ); i++ ){
		if( key == command[ i ].key ){
			command[ i ].function();
		}
	}
}

void ProsessInput(){
	int32_t key = GetInputBufferRead();
	while( key != '\0' ){
		StringFree( &bar_notice );
		if( mode == EDIT_MODE ){
			if( key == ESCAPE_KEY ){
				mode = NORMAL_MODE;
				key = GetInputBuffer();
			} else if( key == TAB_KEY || key == NEWLINE_KEY || ( key >= ' ' && key < DELETE_KEY ) || key & 0x80 ){
				char data[ INPUT_BUFFER_CAP ] = { 0 };
				size_t len = 0;
				do{
					data[ len ] = key;
					len++;
					key = GetInputBuffer();
				} while( key == TAB_KEY || key == NEWLINE_KEY || ( key >= ' ' && key < DELETE_KEY ) || key & 0x80 );
				ProsessEditInsert( data, len );
			} else if( key == DELETE_KEY || key == BACKSPACE_KEY ){
				size_t len = 0;
				do{
					len++;
					key = GetInputBuffer();
				} while( key == DELETE_KEY || key == BACKSPACE_KEY );
				ProsessEditDelete( len );
			} else {
				// control key
				key = GetInputBuffer();
			}
		} else if( mode == NORMAL_MODE ){
			ProsessCommand( key );
			key = GetInputBuffer();
		} else if( mode == FIND_PREV_MODE ){
			mode = NORMAL_MODE;
			if( key != ESCAPE_KEY ){
				ProsessSelectionMoveKey( StringSelectFindCharPrev, key );
			}
			key = GetInputBuffer();
		} else if( mode == FIND_NEXT_MODE ){
			mode = NORMAL_MODE;
			if( key != ESCAPE_KEY ){
				ProsessSelectionMoveKey( StringSelectFindCharNext, key );
			}
			key = GetInputBuffer();
		} else if( mode == SEARCH_MODE ){
			if( key == ESCAPE_KEY ){
				mode = NORMAL_MODE;
				StringFree( &search );
			} else if( key == '\n' ){
				mode = NORMAL_MODE;
				ProsessSearchSubstring();
				StringFree( &search );
			} else if( key == DELETE_KEY || key == BACKSPACE_KEY ){
				StringDeduct( &search, 1 );
			} else {
				StringAppend( &search, (char*) &key, 1 );
			}
			key = GetInputBuffer();
		} else {
			Unreachable();
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
	LoadArgs( argc, argv );
	bool error = StringFromFile( &file, file_name );
	if( error == true ){
		printf( "Can only open regular files\n" );
		exit( 0 );
	}
	if( file.len == 0 || file.data[ file.len - 1 ] != '\n' ){
		StringAppend( &file, "\n", 1 );
	}
	EnableRawMode();
	SelectionNew( 0 );
	while( true ){
		GetScreenSize( &screen.cols, &screen.rows );
		DrawScreen();
		ProsessInput();
		ValidateSelection();
	}
	return 0;
}
