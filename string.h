#ifndef STRING
#define STRING

#include <stdlib.h>
#include <string.h>

#include "assert.h"

#define ALLOC_MULTIPLIER 8

struct String {
        char* data;
        size_t cap;
        size_t len;
};

bool StringAppend( struct String* string, char* src, size_t count );
bool StringInsert( struct String* string, size_t index, char* src, size_t count );
bool StringDeduct( struct String* string, size_t count );
bool StringDelete( struct String* string, size_t index, size_t count );
bool StringAlloc( struct String* string, size_t count );
void StringFree( struct String* string );
bool StringFromFile( struct String* string, char* filename );

bool StringAppend( struct String* string, char* src, size_t count ){
        Assert( string != NULL, "Malformed args" );
        Assert( src != NULL, "Malformed args" );
        Assert( string->cap >= string->len || string->len <= 0, "Malformed internal string data" );
        if( count == 0 ) return true;
        if( string->len + count >= string->cap ){
                size_t cap = string->cap + count * ALLOC_MULTIPLIER;
                char* tmp = realloc( string->data, cap );
                Assert( tmp != NULL, "Alloc failed" );
                string->data = tmp;
                string->cap = cap;
        }
        memmove( &string->data[ string->len ], src, count );
        string->len += count;
        string->data[ string->len ] = '\0';
        return true;
}

bool StringInsert( struct String* string, size_t index, char* src, size_t count ){
        Assert( string != NULL, "Malformed args" );
        Assert( src != NULL, "Malformed args" );
        Assert( string->cap >= string->len || string->len <= 0, "Malformed internal string data" );
        Assert( string->len >= index, "String length is less than index" );
        if( count == 0 ) return true;
        if( string->len + count >= string->cap ){
                size_t cap = string->cap + count * ALLOC_MULTIPLIER;
                char* tmp = realloc( string->data, cap );
                Assert( tmp != NULL, "Alloc failed" );
                string->data = tmp;
                string->cap = cap;
        }
        memmove( &string->data[ index + count ], &string->data[ index ], string->len - index );
        memmove( &string->data[ index ], src, count );
        string->len += count;
        string->data[ string->len ] = '\0';
        return true;
}

bool StringDeduct( struct String* string, size_t count ){
        Assert( string != NULL, "Malformed args" );
        Assert( string->cap >= string->len || string->len <= 0, "Malformed internal string data" );
        Assert( string->len >= count, "String length is less than count" );
        if( count == 0 ) return true;
        string->len -= count;
        string->data[ string->len ] = '\0';
        return true;
}

bool StringDelete( struct String* string, size_t index, size_t count ){
        Assert( string != NULL, "Malformed args" );
        Assert( string->cap >= string->len || string->len <= 0, "Malformed internal string data" );
        Assert( string->len >= index + count, "String length is less than index + count" );
        if( count == 0 ) return true;
        memmove( &string->data[ index ], &string->data[ index + count ], string->len - index - count );
        string->len -= count;
        string->data[ string->len ] = '\0';
        return true;
}

bool StringAlloc( struct String* string, size_t count ){
        Assert( string != NULL, "Malformed args" );
        Assert( string->cap >= string->len || string->len <= 0, "Malformed internal string data" );
        if( count == 0 ) return true;
        char* tmp = realloc( string->data, string->cap + count );
        Assert( tmp != NULL, "Alloc failed" );
        string->data = tmp;
        string->cap += count;
        string->data[ string->len ] = '\0';
        return true;
}

void StringFree( struct String* string ){
        if( string != NULL ){
                free( string->data );
                string->data = NULL;
        }
        string->cap = 0;
        string->len = 0;
}

bool StringFromFile( struct String* string, char* filename ){
        Assert( string != NULL, "Malformed args" );
        Assert( string->cap == 0 && string->len == 0 && string->data == NULL, "String must be empty" );
        Assert( filename != NULL, "Malformed args" );
        FILE* file = fopen( filename, "r" );
        Assert( file != NULL, "fopen failed" );
        int res = fseek( file, 0L, SEEK_END );
        Assert( res == 0, "fseek failed" );
        size_t filelen = ftell( file );
        Assert( filelen > 0, "ftell failed" );
        rewind( file );
        char* tmp = malloc( filelen + 1 );
        Assert( tmp != NULL, "Alloc failed" );
        string->data = tmp;
        size_t fileread = fread( string->data, 1, filelen, file );
        Assert( fileread == filelen, "fread failed" );
        fclose( file );
        string->cap += filelen + 1;
        string->len += filelen;
        string->data[ string->len ] = '\0';
        return true;
}

#undef ALLOC_MULTIPLIER

#endif
