#include "endian_memcpy.h"

/*
 * memcpy, but reverse the order of bytes.
 *
 * Useful for copying integers of differing endianness
 */
void *reverse_memcpy(
        void *restrict dst,
        const void *restrict src,
        size_t n )
{
    size_t i;
    char *d = (char *) dst;
    const char *s = (char *) src;
    for( i = 0; i < n; ++i) d[n - 1 - i] = s[i];
}


