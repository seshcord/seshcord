#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "packet_encoding.h"

#define STREAM stderr

char *uuidtostr( uint8_t *uuid )
{
    static char buffer[33];
    int i;
    for( i = 0; i < 16; i++ ) sprintf( &buffer[i * 2], "%02x", uuid[i] );
    return buffer;
}

void dumppacket( struct seshcord_sv_msg *p )
{
    fprintf( STREAM, "Started dumppacket\n" );
    int i;
    fprintf( STREAM, "ID: %s\n", uuidtostr( p->id ));
    fprintf( STREAM, "Chat: %s\n", uuidtostr( p->chat ));
    fprintf( STREAM, "Sender: %s\n", uuidtostr( p->sender ));
    fprintf( STREAM, "Message: %s\n", p->content );
    fprintf( STREAM, "Attachments: %i\n", p->attachCount );

    for( i = 0; i < p->attachCount; i++ )
    {
        fprintf( STREAM, "---\nAttachment #%i\n", i + 1 );
        fprintf( STREAM, "Filename: %s\n", p->attachments[i].filename );
        fprintf( STREAM, "Size: %i\n", p->attachments[i].size );
        fprintf( STREAM, "Path: %s\n", p->attachments[i].path );
    }
}

void enctest( void )
{
    char buffer[256];
    /* Sample packet */
    struct seshcord_sv_msg test = {
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, /* id */
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  }, /* chat */
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  }, /* sender */
        "This is a message", /* message content */
        2, /* attachment count */
        NULL /* The attachments, we'll fill this in a moment */
    };
    test.attachments = malloc( sizeof( *test.attachments ) * 2 );
    test.attachments[0].filename = "test.txt";
    test.attachments[0].size = 42;
    test.attachments[0].path = "http://test.example.org/test.txt";
    test.attachments[1].filename = "cat.png";
    test.attachments[1].size = 1457664;
    test.attachments[1].path = "http://test.example.org/cat.png";
    int res = encode_from_schema( &test, SESHCORD_SV_MSG_SCHEMA, SESHCORD_SV_MSG_SCHEMA_LEN,
            buffer, sizeof( buffer ), 1 );
    fprintf( STREAM, "size: %i\n", res );
}

void decodetest( void )
{
    /* Create a sample packet, as sent over the wire */
    char testpacket[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* ID UUID */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* chat UUID */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* sender UUID */
        /* 48 */
        'T','h','i','s',' ','i','s',' ',
        'a',' ','m','e','s','s','a','g',
        'e', 0, /* Message */
        /* 66 */
        2, /* Attachment count */
        /* 67 */
        't','e','s','t','.','t','x','t',
        0, /* Attachment 1 filename */
        /* 76 */
        0, 0, 0, 42, /* Attachment 1 size */
        /* 80 */
        'h','t','t','p',':','/','/','t',
        'e','s','t','.','e','x','a','m',
        'p','l','e','.','o','r','g','/',
        't','e','s','t','.','t','x','t',
        /* 112 */
        0, /* Attachment 1 path */
        /* 113 */
        'c','a','t','.','p','n','g', 0,
        /* 121 */
        0, 0x16, 0x3e, 0,
        /* 125 */
        'h','t','t','p',':','/','/','t',
        'e','s','t','.','e','x','a','m',
        'p','l','e','.','o','r','g','/',
        'c','a','t','.','p','n','g', 0,
        /* 157 */
    };

    fprintf( STREAM, "%i\n", sizeof( testpacket ));
    struct seshcord_sv_msg test;
    malloc_group *mal = new_malloc_group( 256 );

    /* Try to decode it: This should work. */
    int res = decode_from_schema( &test, SESHCORD_SV_MSG_SCHEMA,
            SESHCORD_SV_MSG_SCHEMA_LEN, testpacket,
            sizeof( testpacket ), 1, mal );
    if( res < 0 )
    {
        fprintf( STREAM, "Returned with error.\n" );
        free_malloc_group( mal );
        return;
    }
    fprintf( STREAM, "Size: %i\n", res );
    dumppacket( &test );
    fprintf( STREAM, "\n" );

    /* Try to decode it the first N bytes. This should fail since the
     * expected packet is larger than the supplied buffer, The relevant
     * question is, *where* does it fail?
     */
    int partials[] = {
        15, /* This should fail immediately since the first element is
               expected to be 16. */
        16, /* The first UUID should succeed, and then fail after that. */
        65, /* This is one byte short of the null terminator of the first
               string. This should fail reading the string. */
        66, /* This should exactly read the string, and fail on the next
               element. */
    };

    int i;
    for( i = 0; i < sizeof( partials ) / sizeof( partials[0] ); i++ )
    {
        fprintf( STREAM, "First %i test:\n", partials[i] );
        res = decode_from_schema( &test, SESHCORD_SV_MSG_SCHEMA,
                SESHCORD_SV_MSG_SCHEMA_LEN, testpacket,
                partials[i], 1, mal );
        fprintf( STREAM, "Result: %s\n\n", res < 0 ? "Fail" : "Success" );
    }
}

int main( void )
{
    decodetest();

    return 0;
}

/* Dummy callbacks to shut up the linker */
void callback_seshcord_cl_error( struct seshcord_cl_error data ) {}
void callback_seshcord_cl_handshake_init( struct seshcord_cl_handshake_init data ) {}
void callback_seshcord_cl_auth_register( struct seshcord_cl_auth_register data ) {}
void callback_seshcord_cl_auth_login( struct seshcord_cl_auth_login data ) {}
void callback_seshcord_cl_send_msg( struct seshcord_cl_send_msg data ) {}
void callback_seshcord_cl_get_servers( void * data ) {}
void callback_seshcord_cl_get_dms( struct seshcord_cl_get_dms data ) {}
void callback_seshcord_cl_get_friends( void * data ) {}

void callback_seshcord_sv_err( struct seshcord_sv_err data ) {}
void callback_seshcord_sv_msg( struct seshcord_sv_msg data ) {}
void callback_seshcord_sv_msg_del( struct seshcord_sv_msg_del data ) {}
void callback_seshcord_sv_friend_req( struct seshcord_sv_friend_req data) {}
