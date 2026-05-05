#ifndef ASSERT
#define ASSERT

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define Assert( expr, ... ){ \
        if( !( expr )){ \
                fprintf( stderr, "%s:%d: %s: Assertion \"%s\" failed. ", __FILE__, __LINE__, __func__, #expr ); \
                fprintf( stderr, __VA_ARGS__ ); \
                fprintf( stderr, "\r\n" ); \
                fflush( stderr ); \
                abort(); \
        } \
}

#define Unreachable() Assert( 0, "Unreachable." );

#define DebugPrintf( ... ){ \
        fprintf( stderr, "%s:%d: %s ", __FILE__, __LINE__, __func__ ); \
        fprintf( stderr, __VA_ARGS__ ); \
        fflush( stderr ); \
} 

#endif
