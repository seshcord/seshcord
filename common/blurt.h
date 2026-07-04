#include <stdio.h>

/* 
 * A printf-like debugging function.
 *
 * If DEBUG is defined, the blurt() macro calls the _blurt() function. It
 * functions like printf, except it sends its output to stderr.
 *
 * If DEBUG is not defined, _blurt() is #ifdef'd away, and blurt() is a no-op.
 */
#if defined( DEBUG )
int _blurt( char *, ... );
#define blurt( ... ) _blurt( __VA_ARGS__ )
#else
#define blurt( ... )
#endif


