
#define HIGHLIGHT_START BACKGROUND_RED
#define HIGHLIGHT_END BACKGROUND_DEFAULT
#define CURSOR_START INVERSE_START
#define CURSOR_END INVERSE_END

struct Key {
	int32_t key;
	void ( *function )( void );
};

struct Key command[] = {
	/* key function */
	{ ESCAPE_KEY, CommandEscapeNormal },
	{ 'Q', CommandQuit },
	{ 'q', CommandWriteFileQuit },
	{ 'w', CommandWriteFile },
	{ 'i', CommandStandardEditMode },
	{ 'o', CommandNewlineEditMode },
	{ 'F', CommandFindPrevMode },
	{ 'f', CommandFindNextMode },
	{ 'u', CommandEditUndo },
	{ 'U', CommandEditRedo },
	{ 'a', CommandToggleAnchorPin },
	{ 'A', CommandSwapCursorAnchor },
	{ 'h', CommandMoveCharPrev },
	{ 'l', CommandMoveCharNext },
	{ 'b', CommandMoveWordPrev },
	{ 'e', CommandMoveWordNext },
	{ 'k', CommandMoveLinePrev },
	{ 'j', CommandMoveLineNext },
	{ 'm', CommandMoveParagraphPrev },
	{ 'n', CommandMoveParagraphNext },
	{ 'z', CommandMoveLineStart },
	{ 'x', CommandMoveLineEnd },
	{ 't', CommandMoveFileStart },
	{ 'T', CommandMoveFileEnd },
	{ 'g', CommandMoveLineNumber },
	{ 'G', CommandSelectFile },
	{ '(', CommandSelectInsideParen },
	{ ')', CommandSelectInsideParen },
	{ '[', CommandSelectInsideBracket },
	{ ']', CommandSelectInsideBracket },
	{ '{', CommandSelectInsideCurly },
	{ '}', CommandSelectInsideCurly },
	{ '\'', CommandSelectInsideSingleQuote },
	{ '"', CommandSelectInsideDoubleQuote },
	{ 'y', CommandCopySelection },
	{ 'Y', CommandCopyLine },
	{ 'd', CommandDeleteSelection },
	{ 'D', CommandDeleteLine },
	{ 'c', CommandChangeSelection },
	{ 'C', CommandChangeLine },
	{ 'p', CommandPasteClipboard },
	{ 'r', CommandReplaceSelection },
	{ 'R', CommandReplaceLine },
	{ 's', CommandSearchString },
	{ 'S', CommandSearchNewline },
	{ ';', CommandRemoveOtherSelections },
	{ ':', CommandRotateSelection },
	{ '<', CommandDeleteAtLineStart },
	{ '>', CommandInsertTabAtLineStart },
	{ '0', CommandCount0 },
	{ '1', CommandCount1 },
	{ '2', CommandCount2 },
	{ '3', CommandCount3 },
	{ '4', CommandCount4 },
	{ '5', CommandCount5 },
	{ '6', CommandCount6 },
	{ '7', CommandCount7 },
	{ '8', CommandCount8 },
	{ '9', CommandCount9 },
};
