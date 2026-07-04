#include <string.h>

#include "seshcord/endian.h"

void *reverse_memcpy( void *restrict, const void *restrict, size_t );

/*
 * Endian-sensitive memcpy routines. If copying to the same endianness as
 * native, call memcpy. If copying to different endianness, use
 * reverse_memcpy. This is intended for {en,de}coding arbitrary size
 * integers from network packets.
 *
 * n2b_memcpy: Copy from native to big-endian.
 * b2n_memcpy: Copy from big-endian to native.
 * n2l_memcpy: Copy from native to little-endian.
 * l2n_memcpy: Copy from little-endian to native.
 */

#if defined( SESHCORD_CPU_IS_BE )
#define n2b_memcpy memcpy
#define b2n_memcpy memcpy
#define l2n_memcpy reverse_memcpy
#define n2l_memcpy reverse_memcpy
#else
#define n2b_memcpy reverse_memcpy
#define b2n_memcpy reverse_memcpy
#define l2n_memcpy memcpy
#define n2l_memcpy memcpy
#endif

