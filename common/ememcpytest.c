#include <stdint.h>

#include "endian_memcpy.h"

int main( void )
{
    uint32_t o1, o2;
    
    char i1[4] = { 0, 0, 0, 42 };
    char i2[4] = { 69, 0, 0, 0 };

    uint32_t i3 = 666;
    uint32_t i4 = 69420666;
    
    unsigned char o3[4], o4[4];

    b2n_memcpy( &o1, i1, 4 );
    l2n_memcpy( &o2, i2, 4 );
    printf( "%i %i\n", o1, o2);

    n2b_memcpy( o3, &i3, 4 );
    n2l_memcpy( o4, &i4, 4 );
    printf( "%x %x %x %x\n", o3[0], o3[1], o3[2], o3[3] ); 
    printf( "%x %x %x %x\n", o4[0], o4[1], o4[2], o4[3] ); 
    return 0;
}
