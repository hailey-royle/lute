#ifndef STRING
#define STRING

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "assert.h"

struct String {
	char* data;
	size_t cap;
	size_t len;
};

void StringAppend( struct String* string, char* src, size_t count ){
	Assert( string != NULL, "Malformed args" );
	Assert( src != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	if( count == 0 ) return;
	if( string->len + count >= string->cap ){
		size_t cap = ( string->cap + count ) * 2;
		char* tmp = realloc( string->data, cap );
		Assert( tmp != NULL, "Alloc failed" );
		string->data = tmp;
		string->cap = cap;
	}
	memmove( &string->data[ string->len ], src, count );
	string->len += count;
	string->data[ string->len ] = '\0';
	return;
}

void StringInsert( struct String* string, size_t index, char* src, size_t count ){
	Assert( string != NULL, "Malformed args" );
	Assert( src != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( string->len >= index, "String length is less than index" );
	if( count == 0 ) return;
	if( string->len + count >= string->cap ){
		size_t cap = ( string->cap + count ) * 2;
		char* tmp = realloc( string->data, cap );
		Assert( tmp != NULL, "Alloc failed" );
		string->data = tmp;
		string->cap = cap;
	}
	memmove( &string->data[ index + count ], &string->data[ index ], string->len - index );
	memmove( &string->data[ index ], src, count );
	string->len += count;
	string->data[ string->len ] = '\0';
	return;
}

void StringDeduct( struct String* string, size_t count ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( string->len >= count, "String length is less than count" );
	if( count == 0 ) return;
	string->len -= count;
	string->data[ string->len ] = '\0';
	return;
}

void StringDelete( struct String* string, size_t index, size_t count ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( string->len >= index + count, "String length is less than index + count" );
	if( count == 0 ) return;
	memmove( &string->data[ index ], &string->data[ index + count ], string->len - index - count );
	string->len -= count;
	string->data[ string->len ] = '\0';
	return;
}

void StringAlloc( struct String* string, size_t count ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	if( count == 0 ) return;
	char* tmp = realloc( string->data, string->cap + count );
	Assert( tmp != NULL, "Alloc failed" );
	string->data = tmp;
	string->cap += count;
	string->data[ string->len ] = '\0';
	return;
}

void StringFree( struct String* string ){
	Assert( string != NULL, "Malformed args" );
	if( string->data != NULL ){
		free( string->data );
		string->data = NULL;
	}
	string->cap = 0;
	string->len = 0;
}

// for some reason when compiling with -std=* these standard library things dont get included.
// taken from musl c library at musl/include/sys/stat

#ifndef S_IFMT
#define S_IFMT 0170000
#endif

#ifndef S_IFREG
#define S_IFREG 0100000
#endif

void StringFromFile( struct String* string, char* filename ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap == 0 && string->len == 0 && string->data == NULL, "String must be empty" );
	Assert( filename != NULL, "Malformed args" );
	struct stat stat_buffer = { 0 };
	int stat_err = stat( filename, &stat_buffer );
	if( stat_err == -1 ){
		char* tmp = malloc( 1 );
		Assert( tmp != NULL, "Alloc failed." );
		string->data = tmp;
		string->cap = 1;
		string->len = 0;
		string->data[ string->len ] = '\0';
	} else {
		Assert( !(stat_buffer.st_mode & S_IFREG), "Can only edit a regular file." );
		FILE* file = fopen( filename, "r" );
		Assert( file != NULL, "fopen failed" );
		char* tmp = malloc( stat_buffer.st_size + 1 );
		Assert( tmp != NULL, "Alloc failed." );
		string->data = tmp;
		size_t fread_res = fread( string->data, 1, stat_buffer.st_size, file );
		Assert( (size_t) stat_buffer.st_size == fread_res, "fread failed" );
		fclose( file );
		string->cap = stat_buffer.st_size + 1;
		string->len = stat_buffer.st_size ;
		string->data[ string->len ] = '\0';
	}
}

void StringToFile( struct String* string, char* filename ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( filename != NULL, "Malformed args" );
	struct stat stat_buffer = { 0 };
	int stat_err = stat( filename, &stat_buffer );
	if( stat_err != -1 ){
		Assert(( stat_buffer.st_mode & S_IFMT ) == S_IFREG, "Can only write to a regular file." );
	}
	FILE* file = fopen( filename, "w" );
	Assert( file != NULL, "fopen failed" );
	size_t fwrite_res = fwrite( string->data, 1, string->len, file );
	Assert( string->len == fwrite_res, "fwrite failed" );
	fclose( file );
}

size_t StringUTF8Prev( struct String* string, size_t index ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( string->len >= index, "Malformed args" );
	Assert( string->data != NULL, "Malformed args" );
	if( index == 0 ){
		Unreachable();
	}
	index--;
	if(( string->data[ index ] & 0x80 ) == 0 ){
		return index;
	}
	index--;
	if(( string->data[ index ] & 0x40 ) != 0 ){
		return index;
	}
	index--;
	if(( string->data[ index ] & 0x40 ) != 0 ){
		return index;
	}
	index--;
	if(( string->data[ index ] & 0x40 ) != 0 ){
		return index;
	}
	Unreachable();
	return index;
}

size_t StringUTF8Next( struct String* string, size_t index ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( string->len >= index, "Malformed args" );
	Assert( string->data != NULL, "Malformed args" );
	if(( string->data[ index ] & 0x80 ) == 0 ){
		return index + 1;
	}
	if((( string->data[ index ] << 2 ) & 0x80 ) == 0 ){
		return index + 2;
	}
	if((( string->data[ index ] << 3 ) & 0x80 ) == 0 ){
		return index + 3;
	}
	if((( string->data[ index ] << 4 ) & 0x80 ) == 0 ){
		return index + 4;
	}
	Unreachable();
	return index;
}

size_t StringSelectCharPrev( struct String* string, size_t index ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( string->len >= index, "Malformed args" );
	Assert( string->data != NULL, "Malformed args" );
	if( index <= 0 ) {
		return 0;
	}
	return StringUTF8Prev( string, index );
}

size_t StringSelectCharNext( struct String* string, size_t index ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( string->len >= index, "Malformed args" );
	Assert( string->data != NULL, "Malformed args" );
	if( index >= string->len - 1 ){
		return index;
	}
	return StringUTF8Next( string, index );
}

size_t StringSelectWordPrev( struct String* string, size_t index ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( string->len >= index, "Malformed args" );
	Assert( string->data != NULL, "Malformed args" );
	if( index > 0 ){
		index--;
	}
	if( string->data[ index ] == '\n' ){
		return index;
	}
	while( true ){
		if( index <= 0 ){
			break;
		}
		index--;
		if( string->data[ index ] == '\n' ){
			index++;
			break;
		}
		if( string->data[ index ] != ' ' && string->data[ index ] != '\t' ){
			break;
		}
	}
	while( true ){
		if( index <= 0 ){
			break;
		}
		index--;
		if( string->data[ index ] == '\n' ){
			index++;
			break;
		}
		if( string->data[ index ] == ' ' || string->data[ index ] == '\t' ){
			index++;
			break;
		}
	}
	return index;
}

size_t StringSelectWordNext( struct String* string, size_t index ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( string->len >= index, "Malformed args" );
	Assert( string->data != NULL, "Malformed args" );
	if( string->data[ index ] == '\n' ){
		if( index < string->len - 1 ){
			index++;
		}
		return index;
	}
	while( true ){
		if( index >= string->len - 1 ){
			break;
		}
		if( string->data[ index ] == '\n' ){
			break;
		}
		if( string->data[ index ] == ' ' || string->data[ index ] == '\t' ){
			break;
		}
		index++;
	}
	while( true ){
		if( index >= string->len - 1 ){
			break;
		}
		if( string->data[ index ] == '\n' ){
			break;
		}
		if( string->data[ index ] != ' ' && string->data[ index ] != '\t' ){
			break;
		}
		index++;
	}
	return index;
}

size_t StringSelectLineStart( struct String* string, size_t index ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( string->len >= index, "Malformed args" );
	Assert( string->data != NULL, "Malformed args" );
	while( true ){
		if( index <= 0 ){
			break;
		}
		index--;
		if( string->data[ index ] == '\n' ){
			index++;
			break;
		}
	}
	return index;
}

size_t StringSelectLineEnd( struct String* string, size_t index ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( string->len >= index, "Malformed args" );
	Assert( string->data != NULL, "Malformed args" );
	while( true ){
		if( index >= string->len - 1 ){
			break;
		}
		if( string->data[ index ] == '\n' ){
			break;
		}
		index++;
	}
	return index;
}

size_t StringSelectLinePrev( struct String* string, size_t index ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( string->len >= index, "Malformed args" );
	Assert( string->data != NULL, "Malformed args" );
	while( true ){
		if( index <= 0 ){
			break;
		}
		index--;
		if( string->data[ index ] == '\n' ){
			break;
		}
	}
	while( true ){
		if( index <= 0 ){
			break;
		}
		index--;
		if( string->data[ index ] == '\n' ){
			index++;
			break;
		}
	}
	return index;
}

size_t StringSelectLineNext( struct String* string, size_t index ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( string->len >= index, "Malformed args" );
	Assert( string->data != NULL, "Malformed args" );
	while( true ){
		if( index >= string->len - 1 ){
			break;
		}
		if( string->data[ index ] == '\n' ){
			if( index >= string->len - 1 ){
				break;
			}
			index++;
			break;
		}
		index++;
	}
	return index;
}

size_t StringSelectLineNumber( struct String* string, size_t line ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( string->data != NULL, "Malformed args" );
	size_t index = 0;
	while( true ){
		if( line <= 1 ){
			break;
		}
		if( index >= string->len - 1 ){
			break;
		}
		if( string->data[ index ] == '\n' ){
			line--;
		}
		index++;
	}
	return index;
}

size_t StringSelectParagraphPrev( struct String* string, size_t index ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( string->len >= index, "Malformed args" );
	Assert( string->data != NULL, "Malformed args" );
	while( true ){
		if( index <= 0 ){
			break;
		}
		index--;
		if( string->data[ index ] == '\n' ){
			if( index <= 0 ){
				break;
			}
			index--;
			if( string->data[ index ] == '\n' ){
				index++;
				break;
			}
		}
	}
	return index;
}

size_t StringSelectParagraphNext( struct String* string, size_t index ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( string->len >= index, "Malformed args" );
	Assert( string->data != NULL, "Malformed args" );
	while( true ){
		if( index >= string->len - 1 ){
			break;
		}
		if( string->data[ index ] == '\n' ){
			if( index >= string->len - 1 ){
				break;
			}
			index++;
			if( string->data[ index ] == '\n' ){
				break;
			}
		}
		index++;
	}
	return index;
}

size_t StringSelectFindCharPrev( struct String* string, size_t index, char key ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( string->len >= index, "Malformed args" );
	Assert( string->data != NULL, "Malformed args" );
	while( true ){
		if( index <= 0 ){
			break;
		}
		index--;
		if( string->data[ index ] == key ){
			break;
		}
	}
	return index;
}

size_t StringSelectFindCharNext( struct String* string, size_t index, char key ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( string->len >= index, "Malformed args" );
	Assert( string->data != NULL, "Malformed args" );
	while( true ){
		if( index >= string->len - 1 ){
			break;
		}
		index++;
		if( string->data[ index ] == key ){
			break;
		}
	}
	return index;
}

size_t StringSelectSubStringPrev( struct String* string, size_t index, char* key, size_t sub_len ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( string->len >= index, "Malformed args" );
	Assert( string->data != NULL, "Malformed args" );
	Assert( key != NULL, "Malformed args" );
	size_t i = sub_len - 1;
	while( true ){
		if( index <= 0 ){
			break;
		}
		index--;
		if( string->data[ index ] == key[ i ] ){
			if( i == 0 ){
				break;
			}
			i--;
		} else {
			i = sub_len - 1;
		}
	}
	return index;
}

size_t StringSelectSubStringNext( struct String* string, size_t index, char* key, size_t key_len ){
	Assert( string != NULL, "Malformed args" );
	Assert( string->cap > string->len || string->len <= 0, "Malformed internal string data" );
	Assert( string->len >= index, "Malformed args" );
	Assert( string->data != NULL, "Malformed args" );
	Assert( key != NULL, "Malformed args" );
	size_t i = 0;
	if( string->len == 0 ){
		return index;
	};
	while( true ){
		if( index >= string->len - 1 ){
			break;
		}
		index++;
		if( string->data[ index ] == key[ i ] ){
			i++;
			if( i == key_len ){
				index -= key_len - 1;
				break;
			}
		} else {
			i = 0;
		}
	}
	return index;
}

#endif
