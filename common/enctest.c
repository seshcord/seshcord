#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "packet_encoding.h"

int main( void )
{
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

    char buffer[256];
    char testpacket[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* ID UUID */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* chat UUID */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* sender UUID */
        'T','h','i','s',' ','i','s',' ','a',' ',
        'm','e','s','s','a','g','e', 0, /* Message */
        2, /* Attachment coutn */
        't','e','s','t','.','t','x','t', 0, /* Attachment 1 filename */
        0, 0, 0, 42, /* Attachment 1 size */
        'h','t','t','p',':','/','/','t','e','s','t','.',
        'e','x','a','m','p','l','e','.','o','r','g','/',
        't','e','s','t','.','t','x','t', 0, /* Attachment 1 path */
        'c','a','t','.','p','n','g', 0,
        0, 0x16, 0x3e, 0,
        'h','t','t','p',':','/','/','t','e','s','t','.',
        'e','x','a','m','p','l','e','.','o','r','g','/',
        'c','a','t','.','p','n','g', 0
    };

    struct seshcord_sv_msg test2;
    malloc_group *mal = new_malloc_group( 256 );

    int res = decode_from_schema( &test2, SESHCORD_SV_MSG_SCHEMA, SESHCORD_SV_MSG_SCHEMA_LEN,
            testpacket, res, 1, mal );
    fprintf( stderr, "size: %i\n", res );

    fprintf( stderr, "Message: %s\n", test2.content );
    fprintf( stderr, "Attachments: %i\n", test2.attachCount );
    fprintf( stderr, "Attachment 1 path: %s\n", test2.attachments[0].path );

    fprintf( stderr, "Tets packet size: %i\n", sizeof( testpacket ));
    /*
    res = encode_from_schema( &test, SESHCORD_SV_MSG_SCHEMA, SESHCORD_SV_MSG_SCHEMA_LEN,
            buffer, sizeof( buffer ), 1 );
    fprintf( stderr, "size: %i\n", res );
    */

    free_malloc_group( mal );
    return 0;
    fprintf( stderr, "RE-READING\n" );
    /* Write the actual packet to stdout so we can hexdump it and examine it */
    /* fwrite( buffer, res, 1, stdout ); */
    /* struct seshcord_sv_msg test2; */
    res = decode_from_schema( &test2, SESHCORD_SV_MSG_SCHEMA, SESHCORD_SV_MSG_SCHEMA_LEN,
            buffer, res, 1, mal );
    fprintf( stderr, "size: %i\n", res );

    fprintf( stderr, "Message: %s\n", test2.content );
    fprintf( stderr, "Attachments: %i\n", test2.attachCount );
    fprintf( stderr, "Attachment 1 path: %s\n", test2.attachments[0].path );


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
