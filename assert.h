#ifndef ASSERT
#define ASSERT

#include <stdio.h>
#include <stdlib.h>

#ifdef NO_ASSERT
#define Assert( expr, ... ){( void )( expr ); }
#else
#define Assert( expr, ... ){ \
	if( !( expr )){ \
		fprintf( stderr, "%s:%d: %s: Assertion \"%s\" failed. ", __FILE__, __LINE__, __func__, #expr ); \
		fprintf( stderr, __VA_ARGS__ ); \
		fprintf( stderr, "\r\n" ); \
		fflush( stderr ); \
		abort(); \
	} \
}
#endif

#define Unreachable() Assert( 0, "Unreachable." );

#ifdef NO_ASSERT
#define DebugPrintf( ... ){( void )( 0 ); }
#else
#define DebugPrintf( ... ){ \
	fprintf( stderr, "%s:%d: %s ", __FILE__, __LINE__, __func__ ); \
	fprintf( stderr, __VA_ARGS__ ); \
	fflush( stderr ); \
}
#endif

#endif
