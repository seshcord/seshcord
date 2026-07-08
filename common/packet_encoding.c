#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

#include "packet_encoding.h"
#include "endian_memcpy.h"
#include "blurt.h"

/*
 * The different types of types we might want to decode. Makes typecasting
 * slightly cleaner.
 */
union ptype
{
    int8_t int8;
    uint8_t uint8;
    int16_t int16;
    uint16_t uint16;
    int32_t int32;
    uint32_t uint32;
    int64_t int64;
    uint64_t uint64;
    char *str;
    uint64_t time;
    uint8_t uuid[16];
    char *binary;
    void *ptr;
};

/* The physical size of a packet item in its structure */
int ptypesizes[] = {
    1, /* PKT_ITEM_INT8 */
    1, /* PKT_ITEM_UINT8 */
    2, /* PKT_ITEM_INT16 */
    2, /* PKT_ITEM_UINT16 */
    4, /* PKT_ITEM_INT32 */
    4, /* PKT_ITEM_UINT32 */
    8, /* PKT_ITEM_INT64 */
    8, /* PKT_ITEM_UINT64 */
    sizeof( char * ), /* PKT_ITEM_STR */
    8, /* PKT_ITEM_TIME */
    16, /* PKT_ITEM_UUID */
    sizeof( char * ), /* PKT_ITEM_BINARY */
    0, /* PKT_ITEM_LIST */
    0, /* PKT_ITEM_STRUCT */
    0, /* PKT_ITEM_END */
};

/* 
 * The following are helper macros for encode_from_schema(). They reference
 * local variables within that function, and perform some macro magic
 */

/*
 * Copy data from one buffer to another, and update pointers.
 *
 * from: The buffer to copy from.
 * to: The buffer to copy to.
 * size: The cumulative amount of data copied; incremented when copy is
 *      performed.
 * remain: The amount of space remaining in the buffer. If there is inadequate
 *      space, the copy is not performed.
 * len: The number of bytes to copy.
 * incfrom: If true, increase the pointer to the source buffer by the amount
 *      copied.
 * isint: If true, copy from native byte order to big-endian.
 */
void encodebuf( void **from, char **to, int *size, int *remain,
        int len, int incfrom, int isint )
{
    blurt( "Copying %i bytes\n", len );
    *size += len;
    *remain -= len;
    if( *remain >= 0 )
    {
        if( isint )
        {
            n2b_memcpy( *to, *from, len );
        }
        else
        {
            memcpy( *to, *from, len );
        }
        *to += len;
    } 
    if( incfrom ) *from += len;
}

/*
 * The following are helper macros for encode_from_schema. Thery call the
 * encodebuf() function, feeding it local variables from within that function.
 */

/* Copy `s` bytes from `from` to the output buffer */
#define encodebuffrom( from, s ) encodebuf( (void **) &from, \
        &buffer, &size, &remain, s, 0, 0 )

/* Copy the data pointed to by the input buffer, treating it as type `t`,
 * where `t` is a member of the `ptype` union, and not a "type" per se */
#define encodebuft( t ) encodebuf( &input, &buffer, &size, &remain, \
        sizeof( u-> t ), 1, 0 )

/* Copy the data pointed to by the input buffer, save it as an integer
 * in `lastint`, and perform endian-conversion as necessary. */
#define encodebufint( t ) \
    encodebuf( &input, &buffer, &size, &remain, sizeof( u-> t ), 1, 1 ); \
    lastint = u-> t ; \
    blurt( "Decoded an int %i\n", lastint )

/*
 * Encode a packet (payload) for transmission or calculate its size.
 *
 * packet_data: A structure containing decoded packet data
 * schema: The packet schema
 * len: Number of elements in the schema
 * buffer: The buffer to encode the raw binary data to
 * max: The length of the buffer.
 * count: The number of elements to encode. For a sublist, this is the
 *      number of elements, otherwise 1.
 *
 * return: The size of the encoded packet.
 *
 * If the encoded packet is larger than `max`, the buffer is filled out to that
 * length and the rest is discarded. The return value will reflect the actual
 * size required to encode the packet, whether it is fully encoded or not.
 *
 * Thus, specifying NULL for `buffer` and 0 for `max` will calculate the size
 * of the packet without encoding it. This can be useful to determine the
 * amount of memory to allocate a buffer.
 */
int encode_from_schema( void *packet_data, 
        enum packet_items * schema, int len,
        char *buffer, int max, int count )
{
    int i; /* Index into schema */
    void *input = packet_data; /* Pointer to current data item */
    int remain = max; /* Remaining space in the buffer */
    long lastint = 0; /* The previously encoded integer */
    int size = 0; /* Size of the encoded packet */
    int tmp; /* Temporary integer storate */
    int lsize; /* The size of the schema for a sublist */
    union ptype *u; /* The input pointer as a union */
    int j; /* Counter for repeating the encode `count` times */

    for( j = 0; j < count; j++ )
    {
        for( i = 0; i < len; i++ )
        {
            u = input;
            blurt( "Schema item type %i\n", schema[i] );

            switch( schema[i] )
            {
                /* A UUID, just copy the data */
                case PKT_ITEM_UUID: /* 128-bit */
                    encodebuft( uuid );
                    break;

                /* The basic integer types. Save the supplied value in
                 * `lastint` as it may be used to indicate the size of a
                 * sublist or binary blob */
                case PKT_ITEM_INT64:
                    encodebufint( int64 );
                    break;
                case PKT_ITEM_UINT64:
                case PKT_ITEM_TIME:
                    encodebufint( uint64 );
                    break;
                case PKT_ITEM_INT32:
                    encodebufint( int32 );
                    break;
                case PKT_ITEM_UINT32:
                    encodebufint( uint32 );
                    break;
                case PKT_ITEM_INT16:
                    encodebufint( int16 );
                    break;
                case PKT_ITEM_UINT16:
                    encodebufint( uint16 );
                    break;
                case PKT_ITEM_INT8:
                    encodebufint( int8 );
                    break;
                case PKT_ITEM_UINT8:
                    encodebufint( uint8 );
                    break;

                /* Null-terminated string; get its size and copy it. Note that
                 * we're not copying from the input buffer, but from where the
                 * given pointer points. */
                case PKT_ITEM_STR:
                    tmp = strlen( u->str ) + 1;
                    blurt( "Copying a string of size %i\n", tmp );
                    encodebuffrom( u->str, tmp );
                    input += sizeof( u->str );
                    break;

                /* Raw binary data; the previously encoded int specifies its
                 * size. As with str, this isn't copied directly from the input
                 * buffer. */
                case PKT_ITEM_BINARY:
                    encodebuffrom( u->binary, lastint );
                    input += sizeof( u->binary );
                    break;

                /* For sublists, we figure out how many elements a single
                 * instance of the list contains, and recursively call
                 * ourselves, as if the parts of the sublist were their own
                 * packet payload. Like str, this isn't copied from the input
                 * buffer, but from the given pointer. */
                case PKT_ITEM_LIST:
                    lsize = 0; /* list schema size */
                    i++; /* Move past the list start marker */
                    while( schema[i + lsize] != PKT_ITEM_END ) lsize++;
                    blurt( "Processing list of size %i of %i elements\n",
                            lsize,lastint );
                    tmp = encode_from_schema( u->ptr, &schema[i], lsize,
                            buffer, remain, lastint );
                    size += tmp;
                    remain -= tmp;
                    if( remain >= 0 ) buffer += tmp;
                    input += sizeof( u->ptr );
                    i += lsize;
                    break;

                /* Ignore structures, they don't affect the physical format of
                 * the payload */
                case PKT_ITEM_STRUCT:
                    break;

                /* Ignore the end of the structure as well. (PKT_ITEM_END is
                 * absorbed by PKT_ITEM_LIST when used for a list) */
                case PKT_ITEM_END:
                    break;
            } 
        }
    }
    return size;
}

/*
 * Copy data from, incrementing both to point to the next element.
 *
 * from: A double-pointer to the source buffer. The target pointer is
 *      incremented after copying.
 * to: A double-pointer to the destination buffer. The target pointer is
 *      incremented after copying.
 * len: The number of bytes to copy.
 * remain: The number of bytes remaining in the input buffer. If the data
 *      requested to be copied would extend past the end of the buffer, the
 *      function returns with an error.
 * isint: If true, the data to be copied is considered to be an integer in
 *      big-endian format, and will be converted to the native order on copy.
 *
 * Return: True if successful, else false.
 *
 */
int decodebuf( char **from, void **to, int len, int remain, int isint ) 
{
    if( len > remain ) return 0;
    blurt( "Copying %i bytes\n", len ); 
    if( isint )
    {
        b2n_memcpy( *to, *from, len );
    }
    else
    {
        memcpy( *to, *from, len );
    }
    *from += len; 
    *to += len; 
    return 1;
}

/*
 * The following are helper macros for decode_from_schema(). They call
 * decodebuf() with arguments sourced from local variables within the caller.
 */

/*
 * Decode from the input buffer to the output. The `remain` argument to
 * decodebuf() is calculated based on the input pointer. The size argument is
 * calculated based on `t`, which is the name of the relevant member of the
 * `ptype` union. `i` specifies the value of the `isint` apgument. If
 * decodebuf() returns an error, return from decode_from_schema with an error,
 */
#define decodebuft( t, i ) if( ! decodebuf( &input, &output, sizeof( u-> t ), \
            size - (input - buffer), i )) return -1
#define decodebufint( t ) do { decodebuft( t, 1 ); lastint = u-> t; \
    blurt( "Decoded an int %i\n", lastint ); } while( 0 )

/*
 * Decode a received packet (payload)
 *
 * packet_data: A structure tgo decode data into
 * schema: The packet schema
 * len: Number of elements in the schema
 * buffer: The buffer to read raw encoded data from
 * size: The length of the encoded packet
 * count: The number of elements to encode. For a sublist, this is the
 *      number of elements, otherwise 1.
 * mal: A malloc_group that will be used to track memory that this decode
 *      mallocs. The caller is expected to free it when done.
 *
 * return: The number of bytes read from the encoded packet, or negative if
 * an error occurred.
 *
 */
int decode_from_schema( void *packet_data, 
        enum packet_items * schema, int len,
        char *buffer, int size, int count,
        malloc_group *mal )
{
    int i; /* Index into schema */
    char *input = buffer; /* Pointer to current input item */
    void *output = packet_data; /* Pointer to current output item */
    long lastint = 0; /* The previously decoded integer */
    int tmp, tmp2; /* Temporary integer storate */
    int lsize; /* The size of the schema for a sublist */
    union ptype *u; /* The input pointer as a union */
    int j; /* Counter for repeating the decode `count` times */


    for( j = 0; j < count; j++ )
    {
        for( i = 0; i < len; i++ )
        {
            u = output;
            blurt( "Schema item type %i\n", schema[i] );

            switch( schema[i] )
            {
                /* A UUID, just copy the data */
                case PKT_ITEM_UUID: /* 128-bit */
                    decodebuft( uuid, 0 );
                    break;

                /* The basic integer types. Save the supplied value in
                 * `lastint` as it may be used to indicate the size of a
                 * sublist or binary blob */
                case PKT_ITEM_INT64:
                    decodebufint( int64 );
                    break;
                case PKT_ITEM_UINT64:
                case PKT_ITEM_TIME:
                    decodebufint( uint64 );
                    break;
                case PKT_ITEM_INT32:
                    decodebufint( int32 );
                    break;
                case PKT_ITEM_UINT32:
                    decodebufint( uint32 );
                    break;
                case PKT_ITEM_INT16:
                    decodebufint( int16 );
                    break;
                case PKT_ITEM_UINT16:
                    decodebufint( uint16 );
                    break;
                case PKT_ITEM_INT8:
                    decodebufint( int8 );
                    break;
                case PKT_ITEM_UINT8:
                    decodebufint( uint8 );
                    break;

                /* Null-terminated string; get its size and copy it. Note that
                 * we're not copying from the input buffer, but from where the
                 * given pointer points. */
                case PKT_ITEM_STR:
                    tmp = strnlen( input, size - (input - buffer) ) + 1;
                    if( input - buffer + tmp > size )
                    {
                        blurt( "String overran buffer\n" );
                        return -1;
                    }

                    u->str = new_malloc_entry( mal, sizeof( char ) * tmp );
                    strcpy( u->str, input );
                    blurt( "Copying a string of size %i: %s\n", tmp, u->str );
                    output += sizeof( u->str );
                    input += tmp;
                    break;

                /* Raw binary data; the previously encoded int specifies its
                 * size. As with str, this isn't copied directly from the input
                 * buffer. */
                case PKT_ITEM_BINARY:
                    if( input - buffer + lastint > size )
                    {
                        blurt( "Binary blob overran buffer\n" );
                        return -1;
                    }
                    u->binary = new_malloc_entry( mal, lastint );
                    memcpy( u->binary, input, lastint );
                    output += sizeof( u->binary );
                    input += lastint;
                    break;

                /* For sublists, we figure out how many elements a single
                 * instance of the list contains, and recursively call
                 * ourselves, as if the parts of the sublist were their own
                 * packet payload. Like str, this isn't copied from the input
                 * buffer, but from the given pointer. */
                case PKT_ITEM_LIST:
                    lsize = 0; /* list schema size */
                    tmp = 0; /* sublist entry size */
                    i++; /* Move past the list start marker */
                    while( schema[i + lsize] != PKT_ITEM_END )
                    {
                        tmp += ptypesizes[schema[i + lsize]];
                        lsize++;
                    }
                    if( tmp > 0 )
                    {
                        u->ptr = new_malloc_entry( mal, tmp * lsize );
                        blurt( "Processing list of size %i of %i elements\n",
                                lsize,lastint );
                        tmp2 = decode_from_schema( u->ptr, &schema[i], lsize,
                                input, size - (input - buffer), lastint, mal );
                        if( tmp2 < 0 ) return -1;
                        input += tmp2;

                    }
                    else
                    {
                        u->ptr = NULL;
                    }
                    output += sizeof( u->ptr );
                    i += lsize;
                    break;

                /* Ignore structures, they don't affect the physical format of
                 * the payload */
                case PKT_ITEM_STRUCT:
                    break;

                /* Ignore the end of the structure as well. (PKT_ITEM_END is
                 * absorbed by PKT_ITEM_LIST when used for a list) */
                case PKT_ITEM_END:
                    break;
            } 
        }
    }
    return input - buffer;
}

