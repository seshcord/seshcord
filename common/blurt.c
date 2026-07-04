#include <stdarg.h>

#include "blurt.h"

#if defined( DEBUG )
int _blurt( char *fmt, ... )
{
   va_list arg_ptr;

   int result;
   va_start( arg_ptr, fmt );
   result = vfprintf( stderr, fmt, arg_ptr );
   va_end( arg_ptr );
   return result;
}
#endif
