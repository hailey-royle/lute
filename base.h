#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

typedef _Bool i1;

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

void assert_failed( char* file, i32 line, const char* func, char* expression ){
	fprintf( stderr, "%s%s:%d:%s%s \"%s\"\n", ansi_foreground_red, file, line, func, ansi_foreground_default, expression );
	fflush( stderr );
	exit( 1 );
}

#define assert( expression ){\
	if( !( expression )){\
		assert_failed( __FILE__, __LINE__, __func__, #expression );\
	}\
}

#define unreachable(){\
	assert_failed( __FILE__, __LINE__, __func__, "Unreachable reached." );\
}

void error( char* format, ... ){
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

#define make_array( type )\
\
typedef struct {\
	type* data;\
	i32 count;\
	i32 allocated;\
} type ## _array;\
\
void push_ ## type ## _array( type ## _array* array, type push ){\
	assert( array != NULL );\
        if( array->count >= array->allocated ){\
                i32 new_allocated = ( array->allocated == 0 ) ? 16 : array->allocated * 2;\
                type* tmp = realloc( array->data, sizeof( array->data[ 0 ]) * new_allocated );\
		assert( tmp != NULL );\
		array->data = tmp;\
                memset( &array->data[ array->allocated ], 0, sizeof( array->data[ 0 ]) * ( new_allocated - array->allocated ));\
                array->allocated = new_allocated;\
        }\
        array->data[ array->count ] = push;\
        array->count += 1;\
}\
\
void alloc_ ## type ## _array( type ## _array* array, i32 count ){\
        assert( array != NULL );\
        if( array->count + count > array->allocated ){\
                i32 new_allocated = array->count + count;\
                type* tmp = realloc( array->data, sizeof( array->data[ 0 ]) * new_allocated );\
                assert( tmp != NULL );\
                array->data = tmp;\
                memset( &array->data[ array->allocated ], 0, sizeof( array->data[ 0 ]) * ( new_allocated - array->allocated ));\
                array->allocated = new_allocated;\
        }\
}\

/*
i8 is_whitespace( char c ){
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

typedef struct {
        char* data;
        i32 count;
        i32 allocated;
} string;

i8 string_from_file( string* source, char* name, i32 name_length ){
        assert( source != NULL );
        assert( source->data == NULL );
        assert( source->count == 0 );
        assert( source->allocated == 0 );
        assert( name != NULL );
        assert( name_length > 0 );
        char name_null[ name_length + 1 ];
        memcpy( name_null, name, name_length );
        name_null[ name_length ] = '\0';
        FILE* file = fopen( name_null, "w+" );
        if( file == NULL ){
                return 1;
        };
        i32 failed = fseek( file, 0, SEEK_END );
        if( failed != 0 ){
                return 1;
        };
        i32 file_count = ftell( file );
        rewind( file );
        source->data = malloc( file_count + 1 );
        if( source->data == NULL ){
                return 1;
        };
        i64 bytes_read = fread( source->data, 1, file_count, file );
        if( file_count != bytes_read ){
                return 1;
        };
        fclose( file );
        source->allocated = file_count + 1;
        source->count = file_count;
        source->data[ source->count ] = '\0';
        return 0;
}

i8 string_to_file( string* source, char* file_name, i32 name_length ){
        assert( source != NULL );
        assert( source->allocated > source->count || source->count <= 0 );
        assert( file_name != NULL );
        char name[ name_length + 1 ];
        memcpy( name, file_name, name_length );
        name[ name_length ] = '\0';
        FILE* file = fopen( name, "w" );
        if( file == NULL ){
                return 1;
        }
        i64 bytes_wrote = (i64) fwrite( source->data, 1, source->count, file );
        if( source->count != bytes_wrote ){
                return 1;
        }
        fclose( file );
        return 0;
}

void string_free( string* source ){
        assert( source != NULL );
        if( source->data != NULL ){
                free( source->data );
                source->data = NULL;
        }
        source->allocated = 0;
        source->count = 0;
}

void string_alloc( string* source, i32 count ){
        assert( source != NULL );
        if( source->count + count >= source->allocated ){
                i32 new_allocated = source->count + count;
                char* tmp = realloc( source->data, sizeof( source->data[ 0 ]) * new_allocated );
                assert( tmp != NULL );
                source->data = tmp;
                source->allocated = new_allocated;
        }
}

void string_append( string* source, char* src, i32 count ){
        assert( source != NULL );
        assert( src != NULL );
        assert( count >= 0 );
        assert( source->allocated > source->count || source->count <= 0 );
        if( source->count + count >= source->allocated ){
                i32 allocated = ( source->allocated + count ) * 2;
                char* tmp = realloc( source->data, allocated );
                assert( tmp != NULL );
                source->data = tmp;
                source->allocated = allocated;
        }
        memmove( &source->data[ source->count ], src, count );
        source->count += count;
        source->data[ source->count ] = '\0';
        return;
}

void string_insert( string* source, i32 index, char* src, i32 count ){
        assert( source != NULL );
        assert( src != NULL );
        assert( count >= 0 );
        assert( source->count >= index );
        assert( source->allocated > source->count || source->count <= 0 );
        if( source->count + count >= source->allocated ){
                i32 allocated = ( source->allocated + count ) * 2;
                char* tmp = realloc( source->data, allocated );
                assert( tmp != NULL );
                source->data = tmp;
                source->allocated = allocated;
        }
        memmove( &source->data[ index + count ], &source->data[ index ], source->count - index );
        memmove( &source->data[ index ], src, count );
        source->count += count;
        source->data[ source->count ] = '\0';
        return;
}

void string_deduct( string* source, i32 count ){
        assert( source != NULL );
        assert( source->allocated > source->count || source->count <= 0 );
        assert( source->count > count );
        if( count == 0 ){
		return;
	}
        source->count -= count;
        source->data[ source->count ] = '\0';
        return;
}

void string_delete( string* source, i32 index, i32 count ){
        assert( source != NULL );
        assert( source->allocated > source->count || source->count <= 0 );
        assert( source->count >= index + count );
        if( count == 0 ){
		return;
	}
        memmove( &source->data[ index ], &source->data[ index + count ], source->count - index - count );
        source->count -= count;
        source->data[ source->count ] = '\0';
        return;
}

// assumes valid utf8 string
i32 string_next_utf8( string* source, i32 index ){
        assert( source != NULL );
	assert( index >= 0 );
	assert( source->count > index );
	i32 next = index;
	if(( source->data[ index ] & 0x80 ) == 0 ){
		 next += 1;
	} else if((( source->data[ index ] << 2 ) & 0x80 ) == 0 ){
		 next += 2;
	} else if((( source->data[ index ] << 3 ) & 0x80 ) == 0 ){
		 next += 3;
	} else if((( source->data[ index ] << 4 ) & 0x80 ) == 0 ){
		 next += 4;
	} else {
		error( "The file contains an invalid UTF-8 encoding." );
	}
	if( next != source->count ){
		return next;
	} else {
		return index;
	}
}

// assumes valid utf8 string
i32 string_prev_utf8( string* source, i32 index ){
	assert( source != NULL );
	assert( index >= 0 );
	assert( source->count > index );
	if( index == 0 ){
		return 0;
	}
	if(( source->data[ index - 1 ] & 0x80 ) == 0 ){
		return index - 1;
	} else if(( source->data[ index - 2 ] & 0x40 ) != 0 ){
		return index - 2;
	} else if(( source->data[ index - 3 ] & 0x40 ) != 0 ){
		return index - 3;
	} else if(( source->data[ index - 4 ] & 0x40 ) != 0 ){
		return index - 4;
	} else {
		error( "The file contains an invalid UTF-8 encoding." );
	}
	return index;
}

i32 string_next_word( string* source, i32 index ){
	assert( source != NULL );
	assert( index >= 0 );
	assert( source->count > index );
	while( index < source->count - 1 && !is_whitespace( source->data[ index - 1 ])){
		index += 1;
	}
	while( index < source->count - 1 && is_whitespace( source->data[ index - 1 ])){
		index += 1;
	}
	return index;
}

i32 string_prev_word( string* source, i32 index ){
	assert( source != NULL );
	assert( index >= 0 );
	assert( source->count > index );
	while( index > 0 && is_whitespace( source->data[ index - 1 ])){
		index -= 1;
	}
	while( index > 0 && !is_whitespace( source->data[ index - 1 ])){
		index -= 1;
	}
	return index;
}

i32 string_next_line( string* source, i32 index ){
	assert( source != NULL );
	assert( index >= 0 );
	assert( source->count > index );
	while( index < source->count - 1 && source->data[ index ] != '\n' ){
		index += 1;
	}
	if( index < source->count - 1 ){
		index += 1;
	}
	return index;
}

i32 string_prev_line( string* source, i32 index ){
	assert( source != NULL );
	assert( index >= 0 );
	assert( source->count > index );
	while( index > 0 && source->data[ index - 1 ] != '\n' ){
		index -= 1;
	}
	return index;
}

i32 string_next_para( string* source, i32 index ){
	assert( source != NULL );
	assert( index >= 0 );
	assert( source->count > index );
	while( index < source->count - 1 ){
		if( source->data[ index ] == '\n' && index + 1 < source->count - 1 && source->data[ index + 1 ] == '\n' ){
			index += 2;
			while( index < source->count - 1 && source->data[ index ] == '\n' ){
				index += 1;
			}
			break;
		}
		index += 1;
	}
	return index;
}

i32 string_prev_para( string* source, i32 index ){
	assert( source != NULL );
	assert( index >= 0 );
	assert( source->count > index );
	while( index > 0 && source->data[ index - 1 ] == '\n' ){
		index -= 1;
	}
	while( index > 0 ){
		if( source->data[ index - 1 ] == '\n' && index - 1 > 0 && source->data[ index - 2 ] == '\n' ){
			break;
		}
		index -= 1;
	}
	return index;
}

i32 string_next_substring( string* source, i32 index, char* substring, i32 substring_count ){
	assert( source != NULL );
	assert( index >= 0 );
	assert( source->count > index );
	assert( substring_count > 1 );
	i32 start_index = index;
	while( index < source->count - 1 ){
		for( i32 matches = 0; matches <= substring_count; matches += 1 ){
			if( matches == substring_count ){
				return index;
			}
			if( source->data[ index + matches ] != substring[ matches ]){
				break;
			}
		}
		index += 1;
	}
	return start_index;
}

i32 string_prev_substring( string* source, i32 index, char* substring, i32 substring_count ){
	assert( source != NULL );
	assert( index >= 0 );
	assert( source->count > index );
	assert( substring_count > 0 );
	i32 start_index = index;
	while( index > 0 ){
		index -= 1;
		for( i32 matches = 0; matches <= substring_count; matches += 1 ){
			if( matches == substring_count ){
				return index;
			}
			if( source->data[ index + matches ] != substring[ matches ]){
				break;
			}
		}
	}
	return start_index;
}

i32 string_index_from_line( string* source, i32 line ){
	assert( source != NULL );
	assert( line >= 0 );
	i32 current_line = 1;
	i32 index = 0;
	while( index < source->count - 1 && current_line < line ){
		if( source->data[ index ] == '\n' ){
			current_line += 1;
		}
		index += 1;
	}
	return index;
}

i32 string_line_from_index( string* source, i32 index ){
	assert( source != NULL );
	assert( index >= 0 );
	assert( source->count > index );
	i32 line = 1;
	while( index > 0 ){
		index -= 1;
		if( source->data[ index ] == '\n' ){
			line += 1;
		}
	}
	return line;
}
*/

i32 string_to_i32( char* src ){
        i32 ret = 0;
        while( *src >= '0' && *src <= '9' ){
                if( *src != '_' ){
                        ret *= 10;
                        ret += *src & 0xf;
                }
                src += 1;
        }
        return ret;
}

#if defined(_WIN32)
        #error Windows not yet supported.
        #define OS_WINDOWS 1

#elif defined(__gnu_linux__) || defined(__linux__)
        #define OS_LINUX 1

        #include <unistd.h>
        #include <pthread.h>
	#include <termios.h>
	#include <sys/ioctl.h>

#elif defined(__APPLE__) && defined(__MACH__)
        #error Mac not yet supported.
        #define OS_MAC 1

#else
        #error Unknown operating system.
#endif

#if defined( OS_LINUX )

#define thread pthread_t
#define barrier pthread_barrier_t
#define mutex pthread_mutex_t

i64 result_cpu_count = 0;

i64 find_cpu_count(){
	if( result_cpu_count == 0 ){
	        result_cpu_count = sysconf( _SC_NPROCESSORS_ONLN );
	}
	assert( result_cpu_count > 0 );
	return result_cpu_count;
}

void thread_create( thread* thread, void* (start_routine)( void* ), void* arg ){
        i32 failed = pthread_create( thread, NULL, start_routine, arg );
        assert( failed == 0 );
}

void thread_join( thread thread, void* thread_return ){
        i32 failed = pthread_join( thread, thread_return );
        assert( failed == 0 );
}

void barrier_init( pthread_barrier_t* barrier, i32 count ){
        pthread_barrier_init( barrier, NULL, count );
}

void barrier_wait( barrier* barrier ){
        pthread_barrier_wait( barrier );
}

void mutex_init( mutex* mutex ){
        pthread_mutex_init( mutex, NULL );
}

void mutex_lock( mutex* mutex ){
        pthread_mutex_lock( mutex );
}

i32 mutex_trylock( mutex* mutex ){
        return pthread_mutex_trylock( mutex );
}

void mutex_unlock( mutex* mutex ){
        pthread_mutex_unlock( mutex );
}

#endif // linux

