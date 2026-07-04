#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

#include "packet_encoding.h"

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

void ptui( char *, ... );

/* 
 * The following are helper macros for encode_from_schema(). They reference
 * local variables within that function, and perform some macro magic
 */


/* Copy `s` bytes from `from` to the output buffer, and adjust the `size`
 * and `remain` counters accordingly */
/* FIXME: Deal with endianness */
#define copybuffrom( from, s ) \
    fprintf( stderr, "Copying %i bytes\n", s ); \
    size += s; \
    remain -= s; \
    if( remain >= 0 ) { \
        memcpy( buffer, from, s ); \
        buffer += s; \
    } 

/* Copy `s` bytes from the input pointer, and also update it */
#define copybuf( s ) \
    copybuffrom( input, s ) \
    input += s;

/* Copy the data pointed to by the input buffer, treating it as type `t`,
 * where `t` is a member of the `ptype` union, and not a "type" per se */
#define copybuft( t ) \
    copybuf( sizeof( u-> t ));

/* Copy the data pointed to by the input buffer, and save it as an integer
 * in `lastint`. */
#define copybufsave( t ) \
    copybuft( t ); \
    lastint = u-> t ; \
    fprintf( stderr, "Decoded an int %i\n", lastint )

/*
 * Encode a packet (payload) for transmission or calculate its size.
 *
 * packet_data: A structure containing decoded packet data
 * schema: The packet schema
 * len: Number of elements in the schema
 * buffer: The buffer to encode the raw binary data to
 * max: The length of the buffer.
 * count: The number of elements to encode. For a sublist, this is the
 * number of elements, otherwise 1.
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
            fprintf( stderr, "Schema item type %i\n", schema[i] );

            switch( schema[i] )
            {
                /* A UUID, just copy the data */
                case PKT_ITEM_UUID: /* 128-bit */
                    copybuft( uuid );
                    break;

                /* The basic integer types. Save the supplied value in
                 * `lastint` as it may be used to indicate the size of a
                 * sublist or binary blob */
                case PKT_ITEM_INT64:
                    copybufsave( int64 );
                    break;
                case PKT_ITEM_UINT64:
                case PKT_ITEM_TIME:
                    copybufsave( uint64 );
                    break;
                case PKT_ITEM_INT32:
                    copybufsave( int32 );
                    break;
                case PKT_ITEM_UINT32:
                    copybufsave( uint32 );
                    break;
                case PKT_ITEM_INT16:
                    copybufsave( int16 );
                    break;
                case PKT_ITEM_UINT16:
                    copybufsave( uint16 );
                    break;
                case PKT_ITEM_INT8:
                    copybufsave( int8 );
                    break;
                case PKT_ITEM_UINT8:
                    copybufsave( uint8 );
                    break;

                /* Null-terminated string; get its size and copy it. Note that
                 * we're not copying from the input buffer, but from where the
                 * given pointer points. */
                case PKT_ITEM_STR:
                    tmp = strlen( u->str ) + 1;
                    fprintf( stderr, "Copying a string of size %i\n", tmp );
                    copybuffrom( u->str, tmp );
                    input += sizeof( u->str );
                    break;

                /* Raw binary data; the previously encoded int specifies its
                 * size. As with str, this isn't copied directly from the input
                 * buffer. */
                case PKT_ITEM_BINARY:
                    copybuffrom( u->binary, lastint );
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
                    fprintf( stderr, "Processing list of size %i of %i elements\n", lsize,lastint );
                    tmp = encode_from_schema( u->ptr, &schema[i], lsize, buffer, remain, lastint );
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
 * Copy data from `from` to `to`, incrementing both to point to the next element.
 *
 */
int _decodebuf( char **from, void **to, int len, int remain, int isint ) 
{
    if( len > remain ) return 0;
    fprintf( stderr, "Copying %i bytes\n", len ); 
    memcpy( *to, *from, len );
    *from += len; 
    *to += len; 
    return 1;
}

/* The following are helper macros for decode_from_schema(). Basically the
 * converse of the macros used for the encode function. */
/* FIXME: These should check if the relevant structure overruns the input
 * buffer and return with error if so */

#define decodebuffrom( from, s ) if( ! _decodebuf( from, &output, s, size - (input - buffer), 0 )) return -1
#define decodebuf( s ) decodebuffrom( &input, s )
#define decodebuft( t ) decodebuf( sizeof( u-> t ))
/* FIXME: Deal with endianness: here */
#define decodebufint( t ) do { decodebuft( t ); lastint = u-> t; fprintf( stderr, "Decoded an int %i\n", lastint ); } while( 0 )

/*
 * Decode a received packet (payload)
 *
int decode_from_schema( void *packet_data, 
        enum packet_items * schema, int len,
        char *buffer, int size, int count )
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
            fprintf( stderr, "Schema item type %i\n", schema[i] );

            switch( schema[i] )
            {
                /* A UUID, just copy the data */
                case PKT_ITEM_UUID: /* 128-bit */
                    decodebuft( uuid );
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
                    /* FIXME: Return with error if this "string" tries to
                     * overrun the input buffer */
                    tmp = strlen( input ) + 1;
                    u->str = new_malloc_entry( mal, sizeof( char ) * tmp );
                    strcpy( u->str, input );
                    fprintf( stderr, "Copying a string of size %i: %s\n", tmp, u->str );
                    output += sizeof( u->str );
                    input += tmp;
                    break;

                /* Raw binary data; the previously encoded int specifies its
                 * size. As with str, this isn't copied directly from the input
                 * buffer. */
                case PKT_ITEM_BINARY:
                    /* FIXME: Return with error if this blob is larger than
                     * the remaining size of the input buffer */
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
                        fprintf( stderr, "Processing list of size %i of %i elements\n", lsize,lastint );
                        tmp2 = decode_from_schema( u->ptr, &schema[i], lsize, input, size - (input - buffer), lastint, mal );
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

/* Debugging output */

void ptui( char *fmt, ... )
{
   va_list arg_ptr;

   va_start( arg_ptr, fmt );
   vfprintf( stderr, fmt, arg_ptr );
   va_end( arg_ptr );
}
