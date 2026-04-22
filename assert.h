#ifndef ASSERT
#define ASSERT

#include <stdio.h>
#include <stdlib.h>

#define Assert( expr, message ) if (!(expr)) { \
        fprintf( stderr, "%s:%d: %s: Assertion \"%s\" failed. %s\n", \
        __FILE__, __LINE__, __func__, #expr, #message ); \
        abort(); }

#define Unreachable() Assert( 0, "Unreachable." );

#define Todo( message ) Assert( 0, message );

#endif
